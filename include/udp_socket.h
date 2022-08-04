#pragma once

#include <arpa/inet.h>

#define MAX_IP_ADDR_LEN 16

#define SENDBUF_SIZE 2048
#define RECVBUF_SIZE 16384

struct UdpSocketConfig
{
	char ip_address[MAX_IP_ADDR_LEN];
	int socket_buf_size;
	in_port_t port;
	struct sockaddr_in remote;
	unsigned int receive_timeout_usec;
};

struct UdpSocket
{
	int socket;
	struct UdpSocketConfig config;
	uint8_t sendbuf[SENDBUF_SIZE];
	uint8_t recvbuf[RECVBUF_SIZE];
	size_t sendlen;
	size_t recvlen;
	ssize_t receivedlen;
};

struct UdpSocket *udp_socket_new(void);
void udp_socket_destroy(struct UdpSocket *);
int udp_socket_init(struct UdpSocket *);
int udp_socket_config(struct UdpSocket *, char *, int);
void udp_socket_close(struct UdpSocket *);
ssize_t udp_socket_send(struct UdpSocket *);
ssize_t udp_socket_send_to_port(struct UdpSocket *, int);
int udp_socket_has_pending_datagram(struct UdpSocket *);
ssize_t udp_socket_pending_datagram_size(struct UdpSocket *);
int udp_socket_wait_for_read(struct UdpSocket *, int);
int udp_socket_select(struct UdpSocket *, int);
ssize_t udp_socket_receive(struct UdpSocket *, size_t);

void udp_sendbuf_reset(struct UdpSocket *);
void udp_sendbuf_push8(struct UdpSocket *, uint8_t);
void udp_sendbuf_push16(struct UdpSocket *, uint16_t);
void udp_sendbuf_push32(struct UdpSocket *, uint32_t);
