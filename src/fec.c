#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <udp_socket.h>
#include <fec.h>

struct Fec *
fec_new()
{
	struct Fec *fec = (struct Fec *)malloc(sizeof(struct Fec));
	assert(fec != NULL);

	fec->socket = udp_socket_new();
	fec->packet_counter = 0;
	return fec;
}

void
fec_destroy(struct Fec *self)
{
	udp_socket_destroy(self->socket);
	free(self);
}

int
fec_open(struct Fec *self, char *ip_addr, int port)
{
	printf("Opening connection to FEC at addr '%s', port %d\n", ip_addr,
	    port);
	udp_socket_config(self->socket, ip_addr, port);
	udp_socket_init(self->socket);

	return 0;
}

void
fec_close(struct Fec *self)
{
	printf("Closing connection to FEC.\n");
	udp_socket_close(self->socket);
}

void
fec_prepare_send_buffer(struct Fec *self, uint8_t cmd, uint8_t type,
    uint16_t length)
{
	struct UdpSocket *socket = self->socket;
	uint32_t counter = self->packet_counter++ + 0x80000000;
	uint16_t channel_map = 1; /* TODO: Needs to be read from somewhere */

	udp_sendbuf_reset(socket);
	udp_sendbuf_push32(socket, counter);
	udp_sendbuf_push16(socket, 0);
	udp_sendbuf_push16(socket, channel_map);
	udp_sendbuf_push8(socket, cmd);
	udp_sendbuf_push8(socket, type);
	udp_sendbuf_push16(socket, length);
}

int
fec_read_link_status(struct Fec *self)
{
	ssize_t rc;
	struct UdpSocket *socket = self->socket;

	fec_prepare_send_buffer(self, 0xbb, 0xaa, 0xffff);
	udp_sendbuf_push32(socket, 0);
	udp_sendbuf_push32(socket, 16);

	rc = udp_socket_send_to_port(socket, FEC_DEFAULT_VMMAPP_PORT);

	udp_socket_wait_for_read(socket, 1000);

	while (udp_socket_has_pending_datagram(socket)) {
		ssize_t size;
		size = udp_socket_pending_datagram_size(socket);
		if (size < 0) {
			abort();
		} else {
			uint32_t ids;
			rc = udp_socket_receive(socket, (size_t)size);
			(void)rc;

			ids = ntohl(*(uint32_t *)(socket->recvbuf + 20));
			printf("VMM link status: 0x%08x\n", ids);
		}
	}
	return 0;
}

int
fec_read_system_registers(struct Fec *self)
{
	ssize_t rc;
	struct UdpSocket *socket = self->socket;
	unsigned int i;

	fec_prepare_send_buffer(self, 0xbb, 0xaa, 0xffff);
	udp_sendbuf_push32(socket, 0);

	for (i = 0; i < 16; ++i) {
		udp_sendbuf_push32(socket, i);
	}

	rc = udp_socket_send(socket);
	assert(rc > 0);

	udp_socket_wait_for_read(socket, 1000);

	while (udp_socket_has_pending_datagram(socket)) {
		ssize_t size;
		size = udp_socket_pending_datagram_size(socket);
		if (size < 0) {
			abort();
		} else {
			rc = udp_socket_receive(socket, (size_t)size);

			fec_decode_system_parameters(self);
		}
	}

	return 0;
}

void
fec_decode_system_parameters(struct Fec *self)
{
	uint8_t *buf = self->socket->recvbuf;
	uint32_t firmware;
	uint32_t mac_vendor;
	uint32_t mac_device;
	uint32_t fec_ip;
	uint16_t udp_data;
	uint16_t udp_sc;
	uint16_t udp_delay;
	uint16_t date_flow;
	uint16_t eth;
	uint16_t sc_mode;
	uint32_t daq_ip;
	uint32_t dtc_link;
	uint32_t main_clock;
	uint32_t main_clock_status;
	uint32_t firmware_reg;

	firmware = ntohl(*(uint32_t *)(buf + 20));
	mac_vendor = ntohl(*(uint32_t *)(buf + 28));
	mac_device = ntohl(*(uint32_t *)(buf + 36));
	fec_ip = ntohl(*(uint32_t *)(buf + 44));
	udp_data = ntohs(*(uint16_t *)(buf + 54));
	udp_sc = ntohs(*(uint16_t *)(buf + 62));
	udp_delay = ntohs(*(uint16_t *)(buf + 70));
	date_flow = ntohs(*(uint16_t *)(buf + 78));
	eth = ntohs(*(uint16_t *)(buf + 86));
	sc_mode = ntohs(*(uint16_t *)(buf + 94));
	daq_ip = ntohl(*(uint32_t *)(buf + 100));
	dtc_link = ntohl(*(uint32_t *)(buf + 108));
	main_clock = ntohl(*(uint32_t *)(buf + 116));
	main_clock_status = ntohl(*(uint32_t *)(buf + 124));
	firmware_reg = ntohl(*(uint32_t *)(buf + 140));

	printf("firmware = 0x%08x\n", firmware);
	printf("mac_vendor = 0x%08x\n", mac_vendor);
	printf("mac_device = 0x%08x\n", mac_device);
	printf("fec_ip = 0x%08x (%d.%d.%d.%d)\n", fec_ip,
	    ((uint8_t*)&fec_ip)[3],
	    ((uint8_t*)&fec_ip)[2],
	    ((uint8_t*)&fec_ip)[1],
	    ((uint8_t*)&fec_ip)[0]);
	printf("udp_data_port = 0x%04x (dec: %d)\n", udp_data, udp_data);
	printf("udp_sc_port = 0x%04x (dec: %d)\n", udp_sc, udp_sc);
	printf("udp_frame_delay = 0x%04x\n", udp_delay);
	printf("date_flow_par = 0x%04x\n", date_flow);
	printf("eth_ctrl_reg = 0x%04x\n", eth);
	printf("sc_ctrl_reg = 0x%04x\n", sc_mode);
	printf("daq_dest_ip = 0x%08x (%d.%d.%d.%d)\n", daq_ip,
	    ((uint8_t*)&daq_ip)[3],
	    ((uint8_t*)&daq_ip)[2],
	    ((uint8_t*)&daq_ip)[1],
	    ((uint8_t*)&daq_ip)[0]);
	printf("dtc_link_ctrl_reg = 0x%08x\n", dtc_link);
	printf("main_clock_select_reg = 0x%08x\n", main_clock);
	printf("main_clock_status = 0x%08x\n", main_clock_status);
	printf("firmware_reg = 0x%08x\n", firmware_reg);
}
