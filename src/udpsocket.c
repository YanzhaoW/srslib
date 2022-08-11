#include <assert.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <udp_socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#include <util.h>

struct UdpSocket *
udp_socket_new()
{
	struct UdpSocket *s = (struct UdpSocket *)malloc(
	    sizeof(struct UdpSocket));
	assert(s != NULL);
	return s;
}

int
udp_socket_config(struct UdpSocket *self, char *ip_addr, int port)
{
	assert(ip_addr != NULL);
	assert(strlen(ip_addr) < MAX_IP_ADDR_LEN);
	strcpy(self->config.ip_address, ip_addr);
	self->config.port = (in_port_t)port;
	self->config.receive_timeout_usec = 50000;
	self->config.socket_buf_size = 0x2000000;
	self->config.debug = 0;
	self->config.address = inet_addr(self->config.ip_address);
	return 0;
}

int
udp_socket_init(struct UdpSocket *self)
{
	int rc;
	struct sockaddr_in addr;
	int flags;

	memset(&addr, 0, sizeof(struct sockaddr_in));

	self->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (self->socket < 0) {
		perror("socket");
		abort();
	}

	/* set non-blocking */
	flags = fcntl(self->socket, F_GETFL);
	fcntl(self->socket, F_SETFL, flags | O_NONBLOCK);
	
	rc = setsockopt(self->socket, SOL_SOCKET, SO_RCVBUF,
		(char *)&self->config.socket_buf_size,
	 	(int)sizeof(self->config.socket_buf_size));
	if (rc < 0) {
		perror("setsockopt(SO_RCVBUF)");
		abort();
	}

#if 0
	{
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = self->config.receive_timeout_usec;
	rc = setsockopt(self->socket, SOL_SOCKET, SO_RCVTIMEO,
		&timeout, sizeof(struct timeval));
	if (rc < 0) {
		perror("setsockopt(SO_RCVTIMEO)");
		abort();
	}
	}
#endif

	addr.sin_family = AF_INET;
	addr.sin_port = (in_port_t)htons(self->config.port);
	addr.sin_addr.s_addr = INADDR_ANY;

	printf("Binding to port %u\n", self->config.port);

	rc = bind(self->socket, (struct sockaddr *)&addr,
		sizeof(struct sockaddr_in));
	if (rc < 0) {
		perror("bind()");
		abort();
	}

	self->config.remote.sin_family = AF_INET;
	self->config.remote.sin_port = htons(self->config.port);
	self->config.remote.sin_addr.s_addr =
	    inet_addr(self->config.ip_address);

	printf("Remote IP address is: %s\n", self->config.ip_address);
	return 0;
}

void
udp_socket_destroy(struct UdpSocket *self)
{
	free(self);
}

void
udp_socket_close(struct UdpSocket *self)
{
	close(self->socket);
	self->socket = 0;
}

void
udp_sendbuf_reset(struct UdpSocket *self)
{
	self->sendlen = 0;
}

void
udp_sendbuf_push8(struct UdpSocket *self, uint8_t val)
{
	if (self->sendlen + 1 < SENDBUF_SIZE) {
		self->sendbuf[self->sendlen] = val;
		self->sendlen += 1;
	} else {
		printf("udp_sendbuf_push8: send buffer full.\n");
		abort();
	}
}

void
udp_sendbuf_push16(struct UdpSocket *self, uint16_t val)
{
	if (self->sendlen + 2 < SENDBUF_SIZE) {
		uint16_t *p = (uint16_t *)&self->sendbuf[self->sendlen];
		*p = htons(val);
		self->sendlen += 2;
	} else {
		printf("udp_sendbuf_push16: send buffer full.\n");
		abort();
	}
}

void
udp_sendbuf_push32(struct UdpSocket *self, uint32_t val)
{
	if (self->sendlen + 4 < SENDBUF_SIZE) {
		uint32_t *p = (uint32_t *)&self->sendbuf[self->sendlen];
		*p = htonl(val);
		self->sendlen += 4;
	} else {
		printf("udp_sendbuf_push32: send buffer full.\n");
		abort();
	}
}

void udp_sendbuf_push_array8(struct UdpSocket *self, uint8_t *array,
    size_t bytes)
{
	if (self->sendlen + bytes < SENDBUF_SIZE) {
		memcpy(&self->sendbuf[self->sendlen], array, bytes);
		self->sendlen += bytes;
	} else {
		printf("udp_sendbuf_push_array8: send buffer full.\n");
		abort();
	}
}

void udp_sendbuf_push_array16(struct UdpSocket *self, uint16_t *array,
    size_t words)
{
	size_t i;
	for (i = 0; i < words; ++i) {
		udp_sendbuf_push16(self, array[i]);
	}
}

void udp_sendbuf_push_array32(struct UdpSocket *self, uint32_t *array,
    size_t words)
{
	size_t i;
	for (i = 0; i < words; ++i) {
		udp_sendbuf_push32(self, array[i]);
	}
}

/* TODO: Cleanup with udp_socket_send */
ssize_t
udp_socket_send_to_port(struct UdpSocket *self, int port)
{
	ssize_t rc;
	struct sockaddr_in remote;

	if (self->sendlen == 0) {
		return 0;
	}

	remote.sin_family = AF_INET;
	remote.sin_port = htons((in_port_t)port);
	remote.sin_addr.s_addr = self->config.address;

	if (self->config.debug == 1) {
		print_hex(self->sendbuf, self->sendlen,
		    "udp_socket_send_to_port", ">>>");
	}

	rc = sendto(self->socket, self->sendbuf, self->sendlen, 0,
	    (struct sockaddr *)&remote,
	    sizeof(struct sockaddr_in));
	if (rc != (ssize_t)self->sendlen) {
		perror("sendto(udp_socket_send)");
		abort();
	}

	udp_sendbuf_reset(self);

	return rc;
}

ssize_t
udp_socket_send(struct UdpSocket *self)
{
	ssize_t rc;

	if (self->sendlen == 0) {
		return 0;
	}

	if (self->config.debug == 1) {
		print_hex(self->sendbuf, self->sendlen,
		    "udp_socket_send", ">>>");
	}

	rc = sendto(self->socket, self->sendbuf, self->sendlen, 0,
	    (struct sockaddr *)&self->config.remote,
	    sizeof(struct sockaddr_in));
	if (rc != (ssize_t)self->sendlen) {
		perror("sendto(udp_socket_send)");
		abort();
	}

	udp_sendbuf_reset(self);

	return rc;
}

int
udp_socket_has_pending_datagram(struct UdpSocket *self)
{
	ssize_t rc;
	char c;
	int res;
	rc = recv(self->socket, &c, 1, MSG_PEEK);
	res = (rc != -1) || errno == EMSGSIZE;
#if 0
	printf("udp_socket_has_pending_datagram: %s\n", res ? "yes" : "no");
#endif
	return res;
}

ssize_t
udp_socket_pending_datagram_size(struct UdpSocket *self)
{
	ssize_t rc;
	char c;
try_again:
	rc = recv(self->socket, &c, 1, MSG_PEEK | MSG_TRUNC);
	if (rc < 0) {
		if (errno == EAGAIN) {
			goto try_again;
		}
		printf("errno = %d\n", errno);
		perror("recv");
		abort();
	}
#if 0
	printf("udp_socket_pending_datagram_size: %ld\n", rc);
#endif
	return rc;
}

int
udp_socket_wait_for_read(struct UdpSocket *self, int milliseconds)
{
	int rc = udp_socket_select(self, milliseconds);
	if (rc <= 0) {
		perror("udp_socket_select");
		abort();
	} else {
#if 0
		printf("udp_socket_wait_for_read: ready\n");
#endif
		return rc;
	}
}

int
udp_socket_select(struct UdpSocket *self, int milliseconds)
{
	int rc;
	fd_set readfds;
	struct timeval tv;

	FD_ZERO(&readfds);
	FD_SET(self->socket, &readfds);

	tv.tv_sec = 0;
	tv.tv_usec = milliseconds * 1000;

	rc = select(self->socket + 1, &readfds, NULL, NULL, &tv);

	if (rc > 0) {
		if (FD_ISSET(self->socket, &readfds)) {
			return rc;
		} else {
			printf("what?\n");
			abort();
		}
	} else if (rc == 0) {
		printf("select timed out.\n");
		abort();
	} else {
		perror("select");
		abort();
	}
}

int
udp_socket_poll(struct UdpSocket *self, int milliseconds)
{
	int rc;
	struct pollfd fd;
	fd.fd = self->socket;
	fd.events = POLLIN;

	rc = poll(&fd, 1, milliseconds);

	if (rc > 0) {
		if (fd.revents & POLLNVAL) {
			errno = EBADF;
			return -1;
		} else {
			return rc;
		}
	} else if (rc == 0) {
		printf("poll timed out.\n");
		abort();
	} else {
		perror("poll");
		abort();
	}
}

ssize_t
udp_socket_receive(struct UdpSocket *self, size_t size)
{
	ssize_t rc;

	assert(size < RECVBUF_SIZE);

	self->recvlen = size;
try_again:
	rc = recv(self->socket, self->recvbuf, self->recvlen, 0);
	if (rc < 0) {
		if (errno == ETIMEDOUT) {
			perror("recv timed out");
			abort();
		}
		if (errno == EAGAIN) {
			goto try_again;
		}
		perror("recv");
		abort();
	}

	self->receivedlen = rc;

	if (self->config.debug == 1) {
		print_hex(self->recvbuf, (size_t)self->receivedlen,
		    "udp_socket_receive", "<<<");
	}

	if ((size_t)self->receivedlen != self->recvlen) {
		printf("read length mismatch (expect %lu, got %lu)\n",
		    self->recvlen, self->receivedlen);
		abort();
	}

	self->recvlen = 0;

	return rc;
}
