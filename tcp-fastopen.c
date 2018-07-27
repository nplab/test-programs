/*-
 * Copyright (c) 2017 Michael Tuexen
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
#if defined(__FreeBSD__)
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
	struct sockaddr_in addr;
#if defined(__APPLE__)
	sa_endpoints_t endpoints;
	struct iovec iov;
	size_t len;
#endif
#if defined(__FreeBSD__)
	const int on = 1;
#endif
	ssize_t n;
	int fd;

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket");
	}
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
#if defined(__APPLE__) || defined(__FreeBSD__)
	addr.sin_len = sizeof(struct sockaddr_in);
#endif
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_port = htons(atoi(argv[2]));
#if defined(__APPLE__)
	endpoints.sae_srcif = 0;
	endpoints.sae_srcaddr = NULL;
	endpoints.sae_srcaddrlen = 0;
	endpoints.sae_dstaddr = (struct sockaddr *)&addr;
	endpoints.sae_dstaddrlen = sizeof(struct sockaddr_in);
	iov.iov_base = req;
	iov.iov_len = strlen(req);
	len = strlen(req);
	if (connectx(fd, &endpoints, SAE_ASSOCID_ANY, CONNECT_DATA_IDEMPOTENT, &iov, 1, &len, NULL) < 0) {
		perror("connectx");
	}
#elif defined(__FreeBSD__)
	if (setsockopt(fd, IPPROTO_TCP, TCP_FASTOPEN, (const void *)&on, (socklen_t)sizeof(int)) < 0) {
		perror("setsockopt");
	}
	if (sendto(fd, req, strlen(req), 0, (const struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("sendto");
	}
#elif defined(__linux__)
	if (sendto(fd, req, strlen(req), MSG_FASTOPEN, (const struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		perror("sendto");
	}
#else
#error "Unsupported platform"
#endif
	do {
		if ((n = recv(fd, buf, sizeof(buf), 0)) < 0) {
			perror("recv");
		} else {
			if (n > 0) {
				write(1, buf, n);
			}
		}
	} while (n > 0);
	if (close(fd) < 0) {
		perror("close");
	}
	return (0);
}
