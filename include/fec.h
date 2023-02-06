#pragma once

#include <stdint.h>

#define FEC_DEFAULT_FEC_PORT		6007
#define FEC_DEFAULT_FEC_SYS_PORT	6023
#define FEC_DEFAULT_VMMAPP_PORT		6600
#define FEC_DEFAULT_DVMI2C_PORT		6601
#define FEC_DEFAULT_S6_PORT		6602
#define FEC_DEFAULT_VMMASIC_PORT	6603
#define FEC_DEFAULT_I2C_PORT		6604
#define FEC_DEFAULT_DAQ_PORT		6606 /* correct ?? */

#define FEC_DEFAULT_IP "10.0.0.2"
#define FEC_DEFAULT_RECEIVE_PORT        6006
#define FEC_DEFAULT_RECEIVE_PORT_ESS    9000

#define FEC_N_HYBRIDS 8
#define FEC_N_VMM 2
#define FEC_N_VMM_CHANNELS 64

#define FEC_CMD_WRITE		0xaa
#define FEC_CMD_READ		0xbb
#define FEC_CMD_TYPE_LIST	0xaa
#define FEC_CMD_TYPE_PAIRS	0xaa
#define FEC_CMD_LENGTH		0xffff

#define FEC_REG_GLOBAL2_START	 0
#define FEC_REG_GLOBAL2_END	 2
#define FEC_REG_CHANNEL_START	 3
#define FEC_REG_CHANNEL_END	66
#define FEC_REG_GLOBAL1_START	67
#define FEC_REG_GLOBAL1_END	69

#define FEC_ADC_CH_TEMPERATURE 2

#define FEC_RW_SUCCESS 0
#define FEC_RW_NO_DATA -1
#define FEC_RW_PACKET_COUNT_MISMATCH -2

#define FEC_DATA_END_OF_FRAME 0xfafafafa
#define FEC_DATA_ID           0x564d3300

#define HYBRID_ADC_TDO 0
#define HYBRID_ADC_PDO 1
#define HYBRID_ADC_MO  2
#define HYBRID_ADC_UNUSED 3

#define I2C_READ 1
#define I2C_WRITE 0

#define COUNTOF(arr)(sizeof(arr)/(sizeof(arr[0])))

struct FecConfig
{
	uint8_t clock_source;
	uint32_t debug_data_format;
	uint32_t latency_reset;
	uint32_t latency_data_max;
	uint32_t latency_data_error;
	uint32_t tp_offset_first;
	uint32_t tp_offset;
	uint32_t tp_latency;
	uint32_t tp_number;
	uint32_t tp_skew;
	uint32_t tp_width;
	uint32_t tp_polarity;
	uint32_t ckbc_skew;
	uint32_t ckbc;
	uint32_t ckdt;
	int debug;
	int break_on_pkt_cnt_mismatch;
	struct Connection
	{
		char *ip_addr;
		int port;
		int daq_port;
	} connection;
};

enum FecState
{
	FEC_STATE_INVALID,
	FEC_STATE_FRESH,
	FEC_STATE_CONFIGURED
};

struct Fec
{
	struct UdpSocket *socket;
	struct FecConfig config;
	uint32_t packet_counter;
	uint8_t channel_map;
	uint8_t hybrid_index;
	uint8_t vmm_index;
	uint8_t adc_channel;
	uint8_t id; /* made from lowest 8 bits of IP address */
	const uint8_t *hybrid_index_map;
	enum FecState state;
	struct Hybrid
	{
		struct Vmm
		{
			struct VmmConfig {
				uint8_t nskipm_i;
				uint8_t sL0cktest;
				uint8_t sL0dckinv;
				uint8_t sL0ckinv;
				uint8_t sL0ena;
				uint8_t truncate;
				uint8_t nskip;
				uint8_t window;
				uint16_t rollover;
				uint16_t l0offset;
				uint16_t offset;
				/* global SPI 0 */
				uint8_t slvs;
				uint8_t s32;
				uint8_t stcr;
				uint8_t ssart;
				uint8_t srec;
				uint8_t stlc;
				uint8_t sbip;
				uint8_t srat;
				uint8_t sfrst;
				uint8_t slvsbc;
				uint8_t slvstp;
				uint8_t slvstk;
				uint8_t slvsdt;
				uint8_t slvsart;
				uint8_t slvstki;
				uint8_t slvsena;
				uint8_t slvs6b;
				uint8_t sL0enaV;
				uint8_t reset1;
				uint8_t reset2;
				/* SPI 1 */
				uint16_t sdt;
				uint16_t sdp10;
				uint8_t sc10b;
				uint8_t sc8b;
				uint8_t sc6b;
				uint8_t s8b;
				uint8_t s6b;
				uint8_t s10b;
				uint8_t sdcks;
				uint8_t sdcka;
				uint8_t sdck6b;
				uint8_t sdrv;
				uint8_t stpp;
				/* SPI 2 */
				uint8_t sp;
				uint8_t sdp;
				uint8_t sbmx;
				uint8_t sbft;
				uint8_t sbfp;
				uint8_t sbfm;
				uint8_t slg;
				uint8_t sm5_sm0;
				uint8_t scmx;
				uint8_t sfa;
				uint8_t sfam;
				uint8_t st;
				uint8_t sfm;
				uint8_t sg;
				uint8_t sng;
				uint8_t stot;
				uint8_t sttt;
				uint8_t ssh;
				uint8_t stc;
				struct VmmChannel {
					uint8_t sc;
					uint8_t sl;
					uint8_t st;
					uint8_t sth;
					uint8_t sm;
					uint8_t smx;
					uint8_t sd;
					uint8_t sz10b;
					uint8_t sz08b;
					uint8_t sz06b;
				} channel[FEC_N_VMM_CHANNELS];
			} config;
		} vmm[FEC_N_VMM];
	} hybrid[FEC_N_HYBRIDS];
	struct I2C
	{
		int sequence;
		uint8_t address;
		int data;
		int reg;
		uint32_t result;
	} i2c;
};

typedef void(*send_buffer_function)(struct Fec *);
typedef void(*recv_buffer_function)(struct Fec *);

struct Fec *fec_new(void);
void	fec_destroy(struct Fec *);
int	fec_open(struct Fec *);
void	fec_close(struct Fec *);
void	fec_configure(struct Fec *);
void	fec_add_vmm3_hybrid(struct Fec *, int);
void	fec_default_config(struct Fec *);
void	fec_vmm_default_config(struct Vmm *);
int	fec_rw(struct Fec *, int, send_buffer_function, recv_buffer_function);
int	fec_rw_fatal(struct Fec *, int, send_buffer_function,
    recv_buffer_function);
void	fec_prepare_i2c_rw(struct Fec *, uint8_t);
void	fec_prepare_send_buffer(struct Fec *, uint8_t, uint8_t, uint16_t);

#define FEC_READ_FUNCTION_DECL(name) \
    int fec_read_##name(struct Fec *); \
    void fec_prepare_##name(struct Fec *); \
    void fec_decode_##name(struct Fec *)

#define FEC_WRITE_FUNCTION_DECL(name) \
    int fec_write_##name(struct Fec *); \
    void fec_prepare_##name(struct Fec *); \
    void fec_decode_##name(struct Fec *)

#define FEC_I2C_SEQ_FUNCTION_DECL(name) \
    int fec_i2c_##name(struct Fec *, int); \
    void fec_prepare_i2c_##name(struct Fec *); \
    void fec_decode_i2c_##name(struct Fec *)

#define FEC_I2C_FUNCTION_DECL(name) \
    uint32_t fec_i2c_##name(struct Fec *, uint8_t, uint8_t, uint8_t); \
    void fec_prepare_i2c_##name(struct Fec *); \
    void fec_decode_i2c_##name(struct Fec *)

#define FEC_READ(name, port) \
    int fec_read_##name(struct Fec *self) \
    { \
	if (self->config.debug == 1) {printf("fec_read_" #name "\n");} \
        return fec_rw(self, port, &fec_prepare_##name, &fec_decode_##name); \
    }

#define FEC_WRITE(name, port) \
    int fec_write_##name(struct Fec *self) \
    { \
	if (self->config.debug == 1) {printf("fec_write_" #name "\n");} \
        return fec_rw(self, port, &fec_prepare_##name, &fec_decode_##name); \
    }

#define FEC_I2C(name, port) \
    uint32_t fec_i2c_##name(struct Fec *self, uint8_t i2c_address, \
	uint8_t reg_value, uint8_t data) \
    { \
	int rc; \
	self->i2c.address = i2c_address; \
	self->i2c.reg = reg_value; \
	self->i2c.data = data; \
	if (self->config.debug == 1) { \
		printf("fec_i2c_" #name \
		    "[hybrid=%d,addr=0x%x,reg=0x%x,data=0x%x]\n", \
		    self->hybrid_index, i2c_address, reg_value, data); \
	} \
        rc = fec_rw(self, port, &fec_prepare_i2c_##name, \
	    &fec_decode_i2c_##name); \
	assert(rc == 0); \
	return self->i2c.result; \
    }

#define FEC_I2C_SEQ(name, port) \
    int fec_i2c_##name(struct Fec *self, int sequence) \
    { \
	self->i2c.sequence = sequence; \
	if (self->config.debug == 1) { \
		printf("fec_i2c_" #name "[sequence=%d]\n", sequence); \
	} \
        return fec_rw(self, port, &fec_prepare_i2c_##name, \
	    &fec_decode_i2c_##name); \
    }

#define FEC_DECODE_DEFAULT(name, length) \
    void fec_decode_##name(struct Fec *self) {\
	    assert(self->socket->receivedlen == length); \
    }

#define FEC_DECODE_I2C_DEFAULT(name, length) \
    void fec_decode_i2c_##name(struct Fec *self) {\
	    assert(self->socket->receivedlen == length); \
    }

FEC_READ_FUNCTION_DECL(link_status);
FEC_READ_FUNCTION_DECL(system_registers);
FEC_WRITE_FUNCTION_DECL(trigger_acq_constants);
FEC_WRITE_FUNCTION_DECL(powercycle_hybrids);
FEC_WRITE_FUNCTION_DECL(reset);
FEC_WRITE_FUNCTION_DECL(acq_on);
FEC_WRITE_FUNCTION_DECL(acq_off);
FEC_WRITE_FUNCTION_DECL(set_mask);
FEC_WRITE_FUNCTION_DECL(configure_hybrid);
FEC_WRITE_FUNCTION_DECL(send_config);
FEC_I2C_SEQ_FUNCTION_DECL(read_adc);
FEC_I2C_FUNCTION_DECL(write);
FEC_I2C_FUNCTION_DECL(read8);
FEC_I2C_FUNCTION_DECL(read32);

/* TODO: */
FEC_WRITE_FUNCTION_DECL(fec_ip); /* writeFECip */
FEC_WRITE_FUNCTION_DECL(daq_ip); /* writeDAQip */

void fec_do_powercycle_hybrids(struct Fec *);
uint32_t fec_do_read_hybrid_firmware(struct Fec *);
uint32_t fec_do_read_geo_pos(struct Fec *);
void fec_do_read_id_chip(struct Fec *, uint32_t *);
void fec_do_read_adc(struct Fec *, uint8_t, uint8_t, uint8_t);
#define fec_read_temperature(fec, hybrid, vmm) \
    fec_do_read_adc(fec, hybrid, vmm, FEC_ADC_CH_TEMPERATURE)
void fec_debug(struct Fec *, int);
void fec_do_send_config(struct Fec *, uint8_t, uint8_t);

uint32_t *fec_global_registers2(struct Fec *, uint8_t, uint8_t, size_t *);
uint32_t *fec_global_registers(struct Fec *, uint8_t, uint8_t, size_t *);
uint32_t *fec_channel_registers(struct Fec *, uint8_t, uint8_t, size_t *);
const struct VmmConfig *fec_vmm_config(struct Fec *, uint8_t, uint8_t);
