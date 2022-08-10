#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <udp_socket.h>
#include <fec.h>
#include <util.h>

struct Fec *
fec_new()
{
	struct Fec *fec = (struct Fec *)malloc(sizeof(struct Fec));
	assert(fec != NULL);

	fec->socket = udp_socket_new();
	fec->packet_counter = 0;
	fec->hybrid_map = 0;
	fec->hybrid_index = 0;

	fec_default_config(fec);

	return fec;
}

void
fec_destroy(struct Fec *self)
{
	udp_socket_destroy(self->socket);
	free(self);
}

void
fec_default_config(struct Fec *self)
{
	self->config.debug_data_format = 0;
	self->config.latency_reset = 47;
	self->config.latency_data_max = 4087;
	self->config.latency_data_error = 8;
	self->config.tp_offset_first = 100;
	self->config.tp_offset = 1000;
	self->config.tp_latency = 65;
	self->config.tp_number = 1;
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
    uint16_t address)
{
	struct UdpSocket *socket = self->socket;
	uint32_t counter = self->packet_counter++ + 0x80000000;

	/*
	 * command: 0xaa = write, 0xbb = read
	 * command type: 0xaa = write_pairs or read_list, 
	 * command length: always 0xffff
	 */

	/*
	 * address:
	 *   vmmapp: hybrid_map
	 *   dvmi2c: i2c_address
	 */

	udp_sendbuf_reset(socket);
	udp_sendbuf_push32(socket, counter);		/* counter */
	udp_sendbuf_push16(socket, 0);			/* zeros */
	udp_sendbuf_push16(socket, address);		/* address */
	udp_sendbuf_push8(socket, cmd);			/* command */
	udp_sendbuf_push8(socket, type);		/* command type */
	udp_sendbuf_push16(socket, FEC_CMD_LENGTH);	/* command length */
}

void
fec_prepare_link_status(struct Fec *self)
{
	uint32_t array[] = {0, 16};
	fec_prepare_send_buffer(self, FEC_CMD_READ, FEC_CMD_TYPE_PAIRS, 1);
	udp_sendbuf_push_array32(self->socket, array, COUNTOF(array));
}

void
fec_prepare_system_registers(struct Fec *self)
{
	uint32_t i;
	fec_prepare_send_buffer(self, FEC_CMD_READ, FEC_CMD_TYPE_PAIRS, 1);
	udp_sendbuf_push32(self->socket, 0);

	for (i = 0; i < 16; ++i) {
		udp_sendbuf_push32(self->socket, i);
	}
}

void
fec_prepare_trigger_acq_constants(struct Fec *self)
{
	size_t len = 17;
	uint32_t *array = util_fill_array32(len, 0,
		1, self->config.debug_data_format,
		2, self->config.latency_reset,
		3, self->config.latency_data_max,
		4, self->config.latency_data_error,
		9, self->config.tp_offset_first,
		10, self->config.tp_offset,
		11, self->config.tp_latency,
		12, self->config.tp_number);
	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS, 1);
	udp_sendbuf_push_array32(self->socket, array, len);
	free(array);
}

/*
 * note: communicating with a PCA9534 chip (8 bit I2C register)
 * note: this does not work with all FEC firmwares.
 * note: not complete. Currently only sets pin as output.
 */
void
fec_prepare_powercycle_hybrids(struct Fec *self)
{
	uint8_t i2c_address = 0x42; /* device address = 0x21 */
	uint32_t array[] = {0, 0, 0x37f};
	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS,
	    (uint16_t)i2c_address);
	udp_sendbuf_push_array32(self->socket, array, COUNTOF(array));
}

void
fec_prepare_reset(struct Fec *self)
{
	uint32_t array[] = {0, 0xffffffff, 0xffff0001};
	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS, 0);
	udp_sendbuf_push_array32(self->socket, array, COUNTOF(array));
}

void
fec_prepare_acq_on(struct Fec *self)
{
	uint32_t array[] = {0, 15, 1};
	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS,
	    self->hybrid_map);
	udp_sendbuf_push_array32(self->socket, array, COUNTOF(array));
}

void
fec_prepare_acq_off(struct Fec *self)
{
	uint32_t array[] = {0, 15, 0};
	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS,
	    self->hybrid_map);
	udp_sendbuf_push_array32(self->socket, array, COUNTOF(array));
}

void
fec_prepare_set_mask(struct Fec *self)
{
	size_t len = 3;
	uint32_t *array = util_fill_array32(len, 0, 8, self->hybrid_map);
	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS,
	    self->hybrid_map);
	udp_sendbuf_push_array32(self->socket, array, len);
	free(array);
}

/*
 * adc_channel:
 *   0: tdo
 *   1: pdo
 *   2: mo
 *   3: unused
 */
void
fec_prepare_i2c_read_adc(struct Fec *self)
{
	size_t len = 3;
	uint8_t adc_channel = 0; /* TODO: Make configurable */
	uint8_t i2c_address = 0x48 + (self->hybrid_index % 2);
	uint8_t hybrid_bit = self->hybrid_index;
	uint8_t hybrid_map = (1 << hybrid_bit);
	uint16_t address;

	uint8_t rw[] = {I2C_WRITE, I2C_WRITE, I2C_READ};

	i2c_address = (i2c_address << 1) | rw[self->i2c.sequence];
	address = ((uint16_t)hybrid_map) << 8 | i2c_address;
	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS,
	    address);

	switch(self->i2c.sequence)
	{
	case 0:
		{
		uint32_t data;
		uint32_t *array;
		data = (1U << 16)
		    | (1 << 15)
		    | (((uint32_t)adc_channel + 4) << 12)
		    | (2 << 9)
		    | (1 << 8)
		    | 131;

		array = util_fill_array32(len, 0, 0, data);
		udp_sendbuf_push_array32(self->socket, array, len);
		free(array);
		}
		break;
	case 1:
		{
		uint32_t array[] = {0, 0, 0};
		udp_sendbuf_push_array32(self->socket, array, COUNTOF(array));
		}
		break;
	case 2:
		{
		uint32_t array[] = {0, 0, 0xaa0000};
		udp_sendbuf_push_array32(self->socket, array, COUNTOF(array));
		}
		break;
	default:
		printf("Sequence %d not implemented.\n", self->i2c.sequence);
		abort();
	}

}

void
fec_prepare_i2c_write(struct Fec *self)
{
	fec_prepare_i2c_rw(self, I2C_WRITE);
}

void
fec_prepare_i2c_read8(struct Fec *self)
{
	fec_prepare_i2c_rw(self, I2C_READ);
}


void
fec_prepare_i2c_rw(struct Fec *self, uint8_t rw)
{
	size_t len = 3;
	uint8_t i2c_address;
	uint8_t hybrid_bit = self->hybrid_index;
	uint8_t hybrid_map = (1 << hybrid_bit);
	uint16_t address;
	uint32_t *array;

	i2c_address = (self->i2c.address << 1) | rw;
	address = ((uint16_t)hybrid_map) << 8 | i2c_address;
	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS,
	    address);

	array = util_fill_array32(len, 0, self->i2c.data, self->i2c.reg);
	udp_sendbuf_push_array32(self->socket, array, len);
	free(array);
}

FEC_READ(system_registers, FEC_DEFAULT_FEC_PORT)
FEC_WRITE(reset, FEC_DEFAULT_FEC_PORT)
FEC_WRITE(powercycle_hybrids, FEC_DEFAULT_DVMI2C_PORT)
FEC_READ(link_status, FEC_DEFAULT_VMMAPP_PORT)
FEC_WRITE(trigger_acq_constants, FEC_DEFAULT_VMMAPP_PORT)
FEC_WRITE(acq_on, FEC_DEFAULT_VMMAPP_PORT)
FEC_WRITE(acq_off, FEC_DEFAULT_VMMAPP_PORT)
FEC_WRITE(set_mask, FEC_DEFAULT_VMMAPP_PORT)
FEC_I2C_SEQ(read_adc, FEC_DEFAULT_I2C_PORT)
FEC_I2C(write, FEC_DEFAULT_I2C_PORT)
FEC_I2C(read8, FEC_DEFAULT_I2C_PORT)

int
fec_rw(struct Fec *self, int port,
    send_buffer_function prepare_send, recv_buffer_function handle_reply)
{
	ssize_t rc;
	struct UdpSocket *socket = self->socket;
	uint32_t packet_count = self->packet_counter;

	prepare_send(self);
	rc = udp_socket_send_to_port(socket, port);
	assert(rc > 0);
	udp_socket_wait_for_read(socket, 1000);

	/* printf("packet_count = %u\n", packet_count); */

	while (udp_socket_has_pending_datagram(socket)) {
		ssize_t size;
		size = udp_socket_pending_datagram_size(socket);
		if (size < 0) {
			abort();
		} else {
			rc = udp_socket_receive(socket, (size_t)size);
			assert(rc > 0);
			assert(ntohl(((uint32_t *)socket->recvbuf)[0]) ==
			    packet_count);
			handle_reply(self);
		}
	}
	return 0;
}

void
fec_decode_link_status(struct Fec *self)
{
	uint32_t ids;
	uint8_t *buf = self->socket->recvbuf;
	ids = ntohl(*(uint32_t *)(buf + 20));
	printf("VMM link status: 0x%08x\n", ids);
}

void
fec_decode_system_registers(struct Fec *self)
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

FEC_DECODE_DEFAULT(trigger_acq_constants, 80)
FEC_DECODE_DEFAULT(powercycle_hybrids, 24)
FEC_DECODE_DEFAULT(reset, 24)
FEC_DECODE_DEFAULT(acq_on, 24)
FEC_DECODE_DEFAULT(acq_off, 24)
FEC_DECODE_DEFAULT(set_mask, 24)
FEC_DECODE_DEFAULT(i2c_write, 24)

void
fec_decode_i2c_read8(struct Fec *self)
{
	uint8_t value;
	assert(self->socket->receivedlen == 24);
        value = self->socket->recvbuf[23];
	printf("register value (8-bit): %u (0x%02x)\n", value, value);
	self->i2c.result = value;
}

void
fec_decode_i2c_read_adc(struct Fec *self)
{
	assert(self->socket->receivedlen == 24);

	if (self->i2c.sequence == 2) {
		uint16_t adc_value = ntohs(
		    ((uint16_t *)self->socket->recvbuf)[11]) >> 4;
		printf("adc_value: %u\n", adc_value);
	}

	/* 
	 * TODO: Need to do some calculation, if
	 * sp != 0 and sm5_sm0 == 1
	 * Then: adc = 1200 - adc
	 */
}

void
fec_do_powercycle_hybrids(struct Fec *self)
{
	fec_write_powercycle_hybrids(self);
}

uint32_t
fec_do_read_hybrid_firmware(struct Fec *self)
{
	uint32_t data;
	int i;
	fec_i2c_write(self, 65, 6, 1);
	data = 0;
	for (i = 0; i < 4; ++i) {
		data = data << 8 | fec_i2c_read8(self, 65, 0, 1);
	}
	return data;
}
