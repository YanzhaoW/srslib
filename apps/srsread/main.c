#include <stdio.h>
#include <assert.h>
#include <fec.h>
#include <string.h>

#include <vmm3_srs_hit.h>
#include <vmm3_srs_header.h>
#include <vmm3_common_data.h>
#include <udp_socket.h>
#include <util.h>

int read_loop(struct Fec *);
void handle_data(struct Fec *);
size_t handle_vmm3(struct Fec *);
size_t handle_vmm3_srs(struct Fec *);
int parse_header_vmm3_srs(struct Fec *, const struct VMM3SRSHeader *);
int parse_hit_vmm3_srs(const struct VMM3SRSHit *, uint8_t);
int parse_data_vmm3_srs(const struct VMM3SRSHit *, uint8_t);
int parse_marker_vmm3_srs(const struct VMM3SRSHit *, uint8_t);
void print_stats(void);

struct Data
{
	struct VMM3CommonData common;
} data;

struct Stats
{
	size_t n_frames;
	size_t n_hits;
	double start_time;
	double last_stat_time;
	double now;
} stats;


int
main(int argc, char *argv[])
{
	struct Fec *fec;
	(void)argc;
	(void)argv;
	printf("srsread\n");

	stats.start_time = time_double();

	fec = fec_new();
	fec_configure(fec);

	fec->config.break_on_pkt_cnt_mismatch = 0;
	fec->config.connection.port = fec->config.connection.daq_port;

	fec_open(fec);

	memset(&data, 0, sizeof(struct Data));
	memset(&stats, 0, sizeof(struct Stats));

	while (1) {
		double now;

		read_loop(fec);

		now = time_double();
		if(now > stats.last_stat_time + 1) {
			stats.now = now;
			print_stats();
			stats.last_stat_time = now;
		}
	}

	fec_close(fec);
	fec_destroy(fec);
	return 0;
}

void
print_stats()
{
	printf("t: %.0f, f: %lu, hits: %lu\n",
	    stats.now, stats.n_frames, stats.n_hits);
}

size_t
handle_vmm3(struct Fec *fec)
{
	(void)fec;
	printf("  handle_vmm3.\n");
	return 0;
}

size_t
handle_vmm3_srs(struct Fec *fec)
{
	size_t n_hits = 0;
	struct UdpSocket *socket = fec->socket;
	size_t size = (size_t)socket->receivedlen;
	uint8_t *p = socket->recvbuf;
	uint8_t *end = socket->recvbuf + size;
	int rc;

	const struct VMM3SRSHeader *h;
	const struct VMM3SRSHit *hit;

	/* printf("  handle_vmm3_srs.\n"); */

	if (size < 4) {
		printf("Received only %lu words. Too few!\n", size);
		goto vmm3_srs_done;
	}

	h = (const struct VMM3SRSHeader *)p;
	rc = parse_header_vmm3_srs(fec, h);
	if (rc != 0) {
		goto vmm3_srs_fail;
	}

	p += VMM3SRSHeaderSize;
	hit = (struct VMM3SRSHit *)p;

	print_hex(socket->recvbuf,
	    (socket->receivedlen>128) ? 128 : (size_t)socket->receivedlen,
	    "handle_vmm3_srs", "<<<");
	while ((const uint8_t *)hit < end) {
		struct VMM3SRSHit hit_swapped;
		hit_swapped.d32 = ntohl(hit->d32);
		hit_swapped.d16 = ntohs(hit->d16);
		rc = parse_data_vmm3_srs(&hit_swapped, fec->id);
		if (rc == 1) {
			n_hits++;
		}
		/* TODO: Data overflow ? */
		hit++;
	}

	data.common.last_frame_counter = data.common.frame_counter;

vmm3_srs_done:
	return n_hits;

vmm3_srs_fail:
	print_hex(socket->recvbuf,
	    (socket->receivedlen>64) ? 64 : (size_t)socket->receivedlen,
	    "handle_vmm3_srs", "<<<");
	return 0;
}

int
parse_header_vmm3_srs(struct Fec *fec, const struct VMM3SRSHeader *h) {
	uint32_t fc_diff;
	size_t payload_size;
	struct UdpSocket *socket = fec->socket;
	size_t size = (size_t)socket->receivedlen;

	data.common.frame_counter = ntohl(h->frame_counter);
	if (data.common.frame_counter == FEC_DATA_END_OF_FRAME) {
		printf("End of frame.\n");
		goto parse_header_done;
	}

	fc_diff = data.common.frame_counter
	    - data.common.last_frame_counter;
	if (fc_diff > 1) {
		printf("Lost frame(s), frame counter now: %u, prev: %u,"
		    " diff: %u\n", data.common.frame_counter,
		    data.common.last_frame_counter, fc_diff);
		/* TODO: handle */
	}

	if (size < VMM3SRSMinimumPayload) {
		printf("Payload too small: %lu < %lu\n",
		    size, VMM3SRSMinimumPayload);
		goto parse_header_fail;
	}

	data.common.data_id = ntohl(h->data_id);
	if ((data.common.data_id & 0xffffff00) != FEC_DATA_ID) {
		printf("Unknown data id 0x%8x.\n", data.common.data_id);
		goto parse_header_fail;
	}

	data.common.fec_id = (data.common.data_id >> 4) & 0xf;
	if (fec->id != data.common.fec_id) {
		printf("FEC ID mismatch: expected %u, got %u\n", fec->id,
		    data.common.fec_id);
		goto parse_header_fail;
	}

	data.common.udp_timestamp = ntohl(h->udp_timestamp);
	data.common.offset_overflow = ntohl(h->offset_overflow);

	assert(size < 0xffffffff);
	payload_size = size - VMM3SRSHeaderSize;
	printf("VMM3SRSHit size: %lu\n", sizeof(struct VMM3SRSHit));
	if ((payload_size % VMM3SRSHitSize) != 0) {
		printf("Invalid payload_size: %lu\n", payload_size);
		goto parse_header_fail;
	}

parse_header_done:
	return 0;

parse_header_fail:
	return -1;
}

int
parse_hit_vmm3_srs(const struct VMM3SRSHit *hit, uint8_t fec_id)
{
	uint8_t over_threshold = (hit->d16 >> 14) & 1;
	uint8_t ch_no = (hit->d16 >> 8) & 0x3f;
	uint8_t tdc = (uint8_t)(hit->d16 & 0xff);
	uint8_t vmm_id = (hit->d32 >> 22) & 0x1f;
	uint8_t trigger_offset = (uint8_t)((hit->d32 >> 27) & 0x1f);
	uint16_t adc = (uint16_t)((hit->d32 >> 12) & 0x3ff);
	uint32_t bcid = gray2bin32(hit->d32 & 0xfff);
	uint16_t bc_counter_high = adc & 0xf;
	uint16_t bc_counter_low = tdc;
	uint16_t bc_counter = (bc_counter_high << 8) | bc_counter_low;
	(void)bc_counter;

	printf("  (data: %08x%04x) fec: %2u vmm: %2u ch: %2u bcid: %4u "
	    "tdc: %3u adc: %4u "
	    "(%s, trigger_offset: %2u)\n", hit->d32, hit->d16, fec_id, vmm_id,
	    ch_no, bcid, tdc, adc,
	    over_threshold ? "OVER_THR" : "--------", trigger_offset);

	return 1;
}

int
parse_marker_vmm3_srs(const struct VMM3SRSHit *hit, uint8_t fec_id)
{
	uint8_t vmm_id = (hit->d16 >> 10) & 0x1f;
	uint64_t ts_lo = hit->d16 & 0x3ff;
	uint64_t ts_hi = hit->d32;
	uint64_t ts = (ts_hi << 10) | ts_lo;
	uint8_t hybrid = vmm_id / 2;
	uint8_t chip = vmm_id & 1;
	(void)hybrid;
	(void)chip;

	printf("  (data: %08x%04x) fec: %2u vmm: %2u ts: %016lx\n",
	    hit->d32, hit->d16, fec_id, vmm_id, ts);
	return 0;
}

int
parse_data_vmm3_srs(const struct VMM3SRSHit *hit, uint8_t fec_id)
{
	uint8_t data_flag = (hit->d16 >> 15) & 1;
	if (data_flag) {
		return parse_hit_vmm3_srs(hit, fec_id);
	} else {
		return parse_marker_vmm3_srs(hit, fec_id);
	}
}

void
handle_data(struct Fec *fec)
{
	size_t n_hits;

	/* printf("handle_data.\n"); */
	if (fec->config.clock_source <= 1) {
		n_hits = handle_vmm3(fec);
	} else {
		n_hits = handle_vmm3_srs(fec);
	}

	stats.n_hits += n_hits;
	stats.n_frames++;

	/* printf("Got frame with %lu hits\n", n_hits); */
}

int
read_loop(struct Fec *fec)
{
	int rc;
	rc = fec_rw(fec, 0, NULL, handle_data);
	if (rc == FEC_RW_NO_DATA) {
		printf("No data from FEC. Acquisition on and signals coming?\n");
	} else if (rc == FEC_RW_PACKET_COUNT_MISMATCH) {
		/* this is fine */
	} else {
		printf("Received data from FEC.\n");
	}
	return 0;
}
