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

	while ((const uint8_t *)hit < end) {
		rc = parse_hit_vmm3_srs(hit, fec->id);
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
	if ((size % VMM3SRSHitSize) != 0) {
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
	(void)hit;
	(void)fec_id;
	return 1;
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
