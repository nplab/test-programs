/*-
 * Copyright (c) 2017 Michael Tuexen
 * Copyright (c) 2018 Felix Weinrank
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#if defined(__APPLE__)
#include <netinet/tcp.h>
#endif

#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	char buf[1024];
	char *req = "GET /cgi-bin/he HTTP/1.0\r\nUser-agent: tcp_fastopen\r\nConnection: close\r\n\r\n";
	struct sockaddr_storage addr;
	struct sockaddr_in *addr4;
	struct sockaddr_in6 *addr6;
	int socket_family;
	size_t addr_len;

	ssize_t n;
#if defined(__APPLE__)
	const int on = 1;
	//const int off = 0;
#endif
	int fd;

	if (argc != 3) {
		printf("usage: tcp-fastopen IP PORT\n");
		exit(EXIT_FAILURE);
	}

	memset(&addr, 0, sizeof(struct sockaddr_storage));
	addr4 = (struct sockaddr_in*) &addr;
	addr6 = (struct sockaddr_in6*) &addr;

	if (inet_pton(AF_INET, argv[1], &(addr4->sin_addr.s_addr)) == 1) {
		socket_family = AF_INET;
		addr_len = sizeof(struct sockaddr_in);

		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(atoi(argv[2]));
#if defined(__APPLE__) || defined(__FreeBSD__)
		addr4->sin_len = sizeof(struct sockaddr_in);
#endif
	} else if (inet_pton(AF_INET6, argv[1], &(addr6->sin6_addr.s6_addr)) == 1) {
		addr_len = sizeof(struct sockaddr_in6);
		socket_family = AF_INET6;
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(atoi(argv[2]));
#if defined(__APPLE__) || defined(__FreeBSD__)
		addr6->sin6_len = sizeof(struct sockaddr_in6);
#endif
	} else {
		printf("inet_pton failed - please enter valid IPv4/IPv6 address!\n");
		return(EXIT_FAILURE);
	}

	if ((fd = socket(socket_family, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
#if defined(__APPLE__)
	if (setsockopt(fd, IPPROTO_TCP, TCP_ENABLE_ECN, (const void *)&on, (socklen_t)sizeof(int)) < 0) {
		perror("setsockopt");
	}
#endif
	if (connect(fd, (const struct sockaddr *)&addr, addr_len) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}
	if (send(fd, req, strlen(req), 0) < 0) {
		perror("send");
	}

	do {
		if ((n = recv(fd, buf, sizeof(buf), 0)) < 0) {
			perror("recv");
			exit(EXIT_FAILURE);
		} else {
			if (n > 0) {
				write(1, buf, n);
			}
		}
	} while (n > 0);

	if (close(fd) < 0) {
		perror("close");
	}
	return (EXIT_SUCCESS);
}
