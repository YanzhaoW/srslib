#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <udp_socket.h>
#include <fec.h>
#include <util.h>

static const uint8_t hybrid_index_map_normal[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const uint8_t hybrid_index_map_swapped[] = {3, 2, 1, 0, 7, 6, 5, 4};

struct Fec *
fec_new()
{
	struct Fec *fec = (struct Fec *)malloc(sizeof(struct Fec));
	size_t h, v;
	assert(fec != NULL);

	fec->socket = udp_socket_new();
	fec->packet_counter = 0;
	fec->channel_map = 0;
	fec->hybrid_index = 0;
	fec->vmm_index = 0;
	fec->adc_channel = 0;
	fec->state = FEC_STATE_FRESH;

	fec_default_config(fec);

	for (h = 0; h < FEC_N_HYBRIDS; ++h) {
		for (v = 0; v < FEC_N_VMM; ++v) {
			fec_vmm_default_config(&fec->hybrid[h].vmm[v]);
		}
	}

	return fec;
}

/* add a hybrid with two VMM chips to the FEC */
/* FEC has 8 connectors, starting at index 0 */
void
fec_add_vmm3_hybrid(struct Fec *self, int index)
{
	assert(index >= 0 && index < FEC_N_HYBRIDS);
	self->channel_map |= (0x3 << index * 2);
}

void
fec_destroy(struct Fec *self)
{
	udp_socket_destroy(self->socket);
	free(self);
}

void
fec_configure(struct Fec *self)
{
	if (self->config.clock_source <= 1) {
		self->hybrid_index_map = hybrid_index_map_normal;
	} else {
		self->hybrid_index_map = hybrid_index_map_swapped;
	}
	self->state = FEC_STATE_CONFIGURED;
}

void
fec_vmm_default_config(struct Vmm *self)
{
	int i;

	self->config.nskipm_i = 0;
	self->config.sL0cktest = 0;
	self->config.sL0dckinv = 0;
	self->config.sL0ckinv = 0;
	self->config.sL0ena = 0;
	self->config.sL0enaV = 0;
	self->config.truncate = 0;
	self->config.nskip = 0;
	self->config.window = 0;
	self->config.rollover = 0;
	self->config.l0offset = 0;
	self->config.offset = 0;
	self->config.s10b = 1;
	self->config.s8b = 1;
	self->config.sbfm = 1;
	self->config.sbip = 1; /* set in the GUI */
	self->config.sc10b = 3;
	self->config.sc8b = 3;
	self->config.scmx = 0;
	self->config.sdcks = 1;
	self->config.sdp10 = 300; /* 300 in GUI */
	self->config.sdt = 300;   /* 300 in GUI */
	self->config.sg = 2;
	self->config.sm5_sm0 = 4; /* 0: pulser DAC, 1: threshold DAC, 2: bandgap refererce, 3: temperature sensor */
	
	for (i = 0; i < FEC_N_VMM_CHANNELS; ++i ) {
		self->config.channel[i].sc = 0;
		self->config.channel[i].sl = 0;
		self->config.channel[i].st = 1;
		self->config.channel[i].sth = 0;
		self->config.channel[i].sm = 0;
		self->config.channel[i].smx = 0;
		self->config.channel[i].sd = 0;
		self->config.channel[i].sz10b = 0;
		self->config.channel[i].sz08b = 0;
		self->config.channel[i].sz06b = 0;
	}
}

void
fec_default_config(struct Fec *self)
{
	self->config.debug_data_format = 0;
	self->config.debug = 0;
	self->config.latency_reset = 47;
	self->config.latency_data_max = 4087;
	self->config.latency_data_error = 8;
	self->config.tp_offset_first = 100;
	self->config.tp_offset = 1000;
	self->config.tp_latency = 65;
	self->config.tp_number = 1;
	self->config.clock_source = 2;
	/* from fec_config_module.cpp */
	self->config.ckdt = 3;
	self->config.ckbc = 2;
	self->config.ckbc_skew = 0;
	/* hybrid */
	self->config.tp_skew = 0;
	self->config.tp_width = 0;
	self->config.tp_polarity = 0;
}

int
fec_open(struct Fec *self, char *ip_addr, int port)
{
	if (self->state < FEC_STATE_CONFIGURED) {
		printf("need to call fec_configure() before opening a "
		    "connection.\n");
		abort();
	}
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
	fec_prepare_send_buffer(self, FEC_CMD_READ, FEC_CMD_TYPE_PAIRS,
	    self->channel_map);
	udp_sendbuf_push_array32(self->socket, array, COUNTOF(array));
}

void
fec_prepare_system_registers(struct Fec *self)
{
	uint32_t i;
	fec_prepare_send_buffer(self, FEC_CMD_READ, FEC_CMD_TYPE_PAIRS,
	    self->channel_map);
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
	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS,
	    self->channel_map);
	udp_sendbuf_push_array32(self->socket, array, len);
	free(array);
}

/* this configures the VMM hybrid parameters common for both VMM chips */
void
fec_prepare_configure_hybrid(struct Fec *self)
{
	size_t len;
	uint16_t hybrid_map = (1 << self->hybrid_index);
	uint32_t *array;

	if (self->config.clock_source == 3) {
		uint32_t test_pulser_config;
		uint32_t clock_bunch_counter_config;
		uint32_t clock_data_config;

		test_pulser_config = (self->config.tp_polarity << 7)
		    | (self->config.tp_width << 4)
		    | self->config.tp_skew;

		clock_bunch_counter_config = (self->config.ckbc_skew << 4)
		    | self->config.ckbc;

		clock_data_config = self->config.ckdt * 2;

		len = 7;
		array = util_fill_array32(len, 0,
		    2, test_pulser_config,
		    7, clock_bunch_counter_config,
		    5, clock_data_config);
	} else {
		len = 7;
		array = util_fill_array32(len, 0,
		    2, self->config.tp_skew,
		    3, self->config.tp_width,
		    4, self->config.tp_polarity);
	}

	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS,
	    hybrid_map);
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
	    self->channel_map);
	udp_sendbuf_push_array32(self->socket, array, COUNTOF(array));
}

void
fec_prepare_acq_off(struct Fec *self)
{
	uint32_t array[] = {0, 15, 0};
	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS,
	    self->channel_map);
	udp_sendbuf_push_array32(self->socket, array, COUNTOF(array));
}

void
fec_prepare_set_mask(struct Fec *self)
{
	size_t len = 3;
	uint32_t *array = util_fill_array32(len, 0, 8, self->channel_map);
	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS,
	    self->channel_map);
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
	uint8_t adc_channel = self->adc_channel;
	uint8_t i2c_address = 0x48 + (self->vmm_index % 2);
	uint8_t hybrid_bit = self->hybrid_index_map[self->hybrid_index];
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
fec_prepare_i2c_read32(struct Fec *self)
{
	fec_prepare_i2c_rw(self, I2C_READ);
}

void
fec_prepare_i2c_rw(struct Fec *self, uint8_t rw)
{
	size_t len = 3;
	uint8_t i2c_address;
	uint8_t hybrid_bit = self->hybrid_index_map[self->hybrid_index];
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

void
fec_prepare_send_config(struct Fec *self)
{
	uint16_t hybrid_map = 0;
	uint32_t *array;
	size_t len;
	hybrid_map = (uint16_t)(self->hybrid_index * 2 + self->vmm_index);

	fec_prepare_send_buffer(self, FEC_CMD_WRITE, FEC_CMD_TYPE_PAIRS,
	    hybrid_map);

	udp_sendbuf_push32(self->socket, 0);

	array = fec_global_registers2(self, self->hybrid_index,
	    self->vmm_index, &len);
	udp_sendbuf_push_array32(self->socket, array, len);
	free(array);

	array = fec_channel_registers(self, self->hybrid_index,
	    self->vmm_index, &len);
	udp_sendbuf_push_array32(self->socket, array, len);
	free(array);

	array = fec_global_registers(self, self->hybrid_index,
	    self->vmm_index, &len);
	udp_sendbuf_push_array32(self->socket, array, len);
	free(array);

	udp_sendbuf_push32(self->socket, 128);
	udp_sendbuf_push32(self->socket, 1);
}

FEC_READ(system_registers, FEC_DEFAULT_FEC_PORT)
FEC_WRITE(reset, FEC_DEFAULT_FEC_PORT)
FEC_WRITE(powercycle_hybrids, FEC_DEFAULT_DVMI2C_PORT)
FEC_READ(link_status, FEC_DEFAULT_VMMAPP_PORT)
FEC_WRITE(trigger_acq_constants, FEC_DEFAULT_VMMAPP_PORT)
FEC_WRITE(acq_on, FEC_DEFAULT_VMMAPP_PORT)
FEC_WRITE(acq_off, FEC_DEFAULT_VMMAPP_PORT)
FEC_WRITE(set_mask, FEC_DEFAULT_VMMAPP_PORT)
FEC_WRITE(configure_hybrid, FEC_DEFAULT_S6_PORT)
FEC_WRITE(send_config, FEC_DEFAULT_VMMASIC_PORT)
FEC_I2C_SEQ(read_adc, FEC_DEFAULT_I2C_PORT)
FEC_I2C(write, FEC_DEFAULT_I2C_PORT)
FEC_I2C(read8, FEC_DEFAULT_I2C_PORT)
FEC_I2C(read32, FEC_DEFAULT_I2C_PORT)

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

	printf("fec_rw: packet_count = %u\n", packet_count);

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

	printf("FEC System registers:\n");
	printf(" firmware = 0x%08x\n", firmware);
	printf(" mac_vendor = 0x%08x\n", mac_vendor);
	printf(" mac_device = 0x%08x\n", mac_device);
	printf(" fec_ip = 0x%08x (%d.%d.%d.%d)\n", fec_ip,
	    ((uint8_t*)&fec_ip)[3],
	    ((uint8_t*)&fec_ip)[2],
	    ((uint8_t*)&fec_ip)[1],
	    ((uint8_t*)&fec_ip)[0]);
	printf(" udp_data_port = 0x%04x (dec: %d)\n", udp_data, udp_data);
	printf(" udp_sc_port = 0x%04x (dec: %d)\n", udp_sc, udp_sc);
	printf(" udp_frame_delay = 0x%04x\n", udp_delay);
	printf(" date_flow_par = 0x%04x\n", date_flow);
	printf(" eth_ctrl_reg = 0x%04x\n", eth);
	printf(" sc_ctrl_reg = 0x%04x\n", sc_mode);
	printf(" daq_dest_ip = 0x%08x (%d.%d.%d.%d)\n", daq_ip,
	    ((uint8_t*)&daq_ip)[3],
	    ((uint8_t*)&daq_ip)[2],
	    ((uint8_t*)&daq_ip)[1],
	    ((uint8_t*)&daq_ip)[0]);
	printf(" dtc_link_ctrl_reg = 0x%08x\n", dtc_link);
	printf(" main_clock_select_reg = 0x%08x\n", main_clock);
	printf(" main_clock_status = 0x%08x\n", main_clock_status);
	printf(" firmware_reg = 0x%08x\n", firmware_reg);
}

FEC_DECODE_DEFAULT(trigger_acq_constants, 80)
FEC_DECODE_DEFAULT(powercycle_hybrids, 24)
FEC_DECODE_DEFAULT(reset, 24)
FEC_DECODE_DEFAULT(acq_on, 24)
FEC_DECODE_DEFAULT(acq_off, 24)
FEC_DECODE_DEFAULT(set_mask, 24)
FEC_DECODE_DEFAULT(i2c_write, 24)
FEC_DECODE_DEFAULT(configure_hybrid, 40)
FEC_DECODE_DEFAULT(send_config, 584)

void
fec_decode_i2c_read8(struct Fec *self)
{
	uint8_t value;
	assert(self->socket->receivedlen == 24);
        value = self->socket->recvbuf[23];
	if (self->config.debug == 1) {
		printf("register value (8-bit): %u (0x%02x)\n", value, value);
	}
	self->i2c.result = value;
}

void
fec_decode_i2c_read32(struct Fec *self)
{
	uint32_t value;
	assert(self->socket->receivedlen == 24);
        value = ntohl(((uint32_t *)self->socket->recvbuf)[5]);
	if (self->config.debug == 1) {
		printf("register value (32-bit): %u (0x%08x)\n", value, value);
	}
	self->i2c.result = value;
}


void
fec_decode_i2c_read_adc(struct Fec *self)
{
	assert(self->socket->receivedlen == 24);

	if (self->i2c.sequence == 2) {
		uint16_t adc_value = ntohs(
		    ((uint16_t *)self->socket->recvbuf)[11]) >> 4;
		self->i2c.result = adc_value;
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

uint32_t
fec_do_read_geo_pos(struct Fec *self)
{
	fec_i2c_write(self, 66, 2, 1);
	return fec_i2c_read8(self, 66, 0, 1);
}

void
fec_do_read_id_chip(struct Fec *self, uint32_t *id /* fixed len 4 */)
{
	uint8_t n;
	for (n = 1; n <= 4; ++n) {
		uint32_t data;
		fec_i2c_write(self, 88, 0x80, 0);
		data = fec_i2c_read32(self, 88, 0, n * 4);
		/* printf("data = %08x\n", data); */
		id[n - 1] = data;
	}
}

void
fec_do_read_adc(struct Fec *self, uint8_t hybrid, uint8_t vmm, uint8_t ch)
{
	self->adc_channel = ch;
	self->hybrid_index = hybrid;
	self->vmm_index = vmm;

	fec_i2c_read_adc(self, 0);
	fec_i2c_read_adc(self, 1);
	fec_i2c_read_adc(self, 2);

	if (ch == FEC_ADC_CH_TEMPERATURE) {
		float t = (float)(725 - self->i2c.result) / 1.85f;
		printf("temperature[hybrid=%u,vmm=%u] = %.1f\n", hybrid, vmm,
		    t);
	} else {
		printf("adc value [%u] = %d\n", ch, self->i2c.result);
	}
}

void
fec_debug(struct Fec *self, int level)
{
	self->config.debug = level;
	self->socket->config.debug = level;
}

void
fec_do_send_config(struct Fec *self, uint8_t hybrid, uint8_t vmm)
{
	self->hybrid_index = hybrid;
	self->vmm_index = vmm;
	fec_i2c_write(self, 65, 0, 2);

	fec_write_send_config(self);
}

uint32_t *
fec_global_registers2(struct Fec *self, uint8_t hybrid, uint8_t vmm,
    size_t *len)
{
	uint32_t *array;
	size_t idx = 0;
	const struct VmmConfig *c; 
	size_t n_regs = FEC_REG_GLOBAL2_END - FEC_REG_GLOBAL2_START + 1;
	size_t size = n_regs * 2;

	c = fec_vmm_config(self, hybrid, vmm);

	array = (uint32_t *)calloc(sizeof(uint32_t), size);
	assert(array != NULL);

	/* magic number of BCID, 31 */
	array[idx++] = FEC_REG_GLOBAL2_START;
	array[idx++] = (c->nskipm_i << 31);

	array[idx++] = FEC_REG_GLOBAL2_START + 1;
	array[idx++] = (c->sL0cktest & 1)
	    | ((c->sL0dckinv & 1) << 1)
	    | ((c->sL0ckinv & 1) << 2)
	    | ((c->sL0ena & 1) << 3)
	    | ((c->truncate & 0x3f) << 4)
	    | ((c->nskip & 0x7f) << 10)
	    | ((c->window & 0x7) << 17)
	    | ((c->rollover & 0xfff) << 20);

	array[idx++] = FEC_REG_GLOBAL2_START + 2;
	array[idx++] = (c->l0offset & 0xfff)
	    | ((c->offset & 0xfff) << 12);

	*len = size;
	return array;
}

uint32_t *
fec_global_registers(struct Fec *self, uint8_t hybrid, uint8_t vmm,
    size_t *len)
{
	uint32_t *array;
	const struct VmmConfig *c; 
	size_t n_regs = FEC_REG_GLOBAL1_END - FEC_REG_GLOBAL1_START + 1;
	size_t size = n_regs * 2;
	size_t i;

	c = fec_vmm_config(self, hybrid, vmm);

	array = (uint32_t *)calloc(sizeof(uint32_t), size);
	assert(array != NULL);

	for (i = 0; i < n_regs; ++i) {
		array[i * 2] = (uint32_t)(FEC_REG_GLOBAL1_START + i);
	}

	/* SPI 0 */
	array[1] = ((0 & 0xf) << 28)
	    | ((c->slvs & 1) << 27)
	    | ((c->s32 & 1) << 26)
	    | ((c->stcr & 1) << 25)
	    | ((c->ssart & 1) << 24)
	    | ((c->srec & 1) << 23)
	    | ((c->stlc & 1) << 22)
	    | ((c->sbip & 1) << 21)
	    | ((c->srat & 1) << 20)
	    | ((c->sfrst & 1) << 19)
	    | ((c->slvsbc & 1) << 18)
	    | ((c->slvstp & 1) << 17)
	    | ((c->slvstk & 1) << 16)
	    | ((c->slvsdt & 1) << 15)
	    | ((c->slvsart & 1) << 14)
	    | ((c->slvstki & 1) << 13)
	    | ((c->slvsena & 1) << 12)
	    | ((c->slvs6b & 1) << 11)
	    | ((c->sL0enaV & 1) << 10)
	    | ((0 & 0xff) << 2)
	    | ((c->reset1 & 1) << 1)
	    | ((c->reset2 & 1) << 0);

	/* SPI 1 */
	array[3] = ((c->sdt & 0x3f) << 26)
	    | ((c->sdp10 & 0x3ff) << 16)
	    | ((c->sc10b & 0x3) << 14)
	    | ((c->sc8b & 0x3) << 12)
	    | ((c->sc6b & 0x7) << 9)
	    | ((c->s8b & 1) << 8)
	    | ((c->s6b & 1) << 7)
	    | ((c->s10b & 1) << 6)
	    | ((c->sdcks & 1) << 5)
	    | ((c->sdcka & 1) << 4)
	    | ((c->sdck6b & 1) << 3)
	    | ((c->sdrv & 1) << 2)
	    | ((c->stpp & 1) << 1);

	/* SPI 2 */
	array[5] = ((c->sp & 1) << 31)
	    | ((c->sdp &   1) << 30)
	    | ((c->sbmx &  1) << 29)
	    | ((c->sbft &  1) << 28)
	    | ((c->sbfp &  1) << 27)
	    | ((c->sbfm &  1) << 26)
	    | ((c->slg &   1) << 25)
	    | ((c->sm5_sm0 & 0x3f) << 19)
	    | ((c->scmx &  1) << 18)
	    | ((c->sfa &   1) << 17)
	    | ((c->sfam &  1) << 16)
	    | ((c->st &  0x3) << 14)
	    | ((c->sfm &   1) << 13)
	    | ((c->sg &  0x7) << 10)
	    | ((c->sng &   1) << 9)
	    | ((c->stot &  1) << 8)
	    | ((c->sttt &  1) << 7)
	    | ((c->ssh &   1) << 6)
	    | ((c->stc & 0x3) << 4)
	    | (((c->sdt >> 6) & 0xf) << 0);

	*len = size;
	return array;
}

uint32_t *
fec_channel_registers(struct Fec *self, uint8_t hybrid, uint8_t vmm,
    size_t *len)
{
	uint32_t *array;
	const struct VmmConfig *c; 
	size_t n_regs = FEC_REG_CHANNEL_END - FEC_REG_CHANNEL_START + 1;
	size_t size = n_regs * 2;
	size_t i;

	c = fec_vmm_config(self, hybrid, vmm);
	(void)c;

	array = (uint32_t *)calloc(sizeof(uint32_t), size);
	assert(array != NULL);

	for (i = 0; i < n_regs; ++i) {
		array[i * 2] = (uint32_t)(FEC_REG_CHANNEL_START + i);
		/* TODO: need to reverse bit order here? */
		array[i * 2 + 1] = ((c->channel[i].sc & 1) << 23)
		    | ((c->channel[i].sl & 1) << 22)
		    | ((c->channel[i].st & 1) << 21)
		    | ((c->channel[i].sth & 1) << 20)
		    | ((c->channel[i].sm & 1) << 19)
		    | ((c->channel[i].smx & 1) << 18)
		    | ((c->channel[i].sd & 0x1f) << 13)
		    | ((c->channel[i].sz10b & 0x1f) << 8)
		    | ((c->channel[i].sz08b & 0xf) << 4)
		    | ((c->channel[i].sz06b & 0x7) << 1);

	}

	*len = size;
	return array;
}

const struct VmmConfig *
fec_vmm_config(struct Fec *self, uint8_t hybrid, uint8_t vmm)
{
	return &self->hybrid[hybrid].vmm[vmm].config;
}
