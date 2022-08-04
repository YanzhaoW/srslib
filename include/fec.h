#pragma once

#include <stdint.h>

#define FEC_DEFAULT_FEC_PORT 6007
#define FEC_DEFAULT_VMMAPP_PORT 6600
#define FEC_DEFAULT_IP "10.0.0.2"

struct FecConfig
{
	int dummy;
};

struct Fec
{
	struct UdpSocket *socket;
	struct FecConfig config;
	uint32_t packet_counter;
};

struct Fec *fec_new(void);
void fec_destroy(struct Fec *);
int fec_open(struct Fec *, char *, int);
void fec_close(struct Fec *);
int fec_read_system_registers(struct Fec *);
int fec_read_link_status(struct Fec *);
void fec_decode_system_parameters(struct Fec *);
void fec_prepare_send_buffer(struct Fec *, uint8_t, uint8_t, uint16_t);
