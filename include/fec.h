#pragma once

#include <stdint.h>

#define FEC_DEFAULT_FEC_PORT		6007
#define FEC_DEFAULT_FEC_SYS_PORT	6023
#define FEC_DEFAULT_VMMAPP_PORT		6600
#define FEC_DEFAULT_DVMI2C_PORT		6601
#define FEC_DEFAULT_S6_PORT		6602
#define FEC_DEFAULT_VMMASIC_PORT	6603
#define FEC_DEFAULT_I2C_PORT		6604
#define FEC_DEFAULT_DAQ_PORT		6606

#define FEC_DEFAULT_IP "10.0.0.2"

#define FEC_CMD_WRITE		0xaa
#define FEC_CMD_READ		0xbb
#define FEC_CMD_TYPE_LIST	0xaa
#define FEC_CMD_TYPE_PAIRS	0xaa
#define FEC_CMD_LENGTH		0xffff

#define FEC_ADC_CH_TEMPERATURE 2

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
	uint8_t hybrid_map;
	uint8_t hybrid_index;
	uint8_t adc_channel;
	const uint8_t *hybrid_index_map;
	enum FecState state;
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
int	fec_open(struct Fec *, char *, int);
void	fec_close(struct Fec *);
void	fec_configure(struct Fec *);
void	fec_default_config(struct Fec *);
int	fec_rw(struct Fec *, int, send_buffer_function, recv_buffer_function);
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
void fec_do_read_adc(struct Fec *, uint8_t);
