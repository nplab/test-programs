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

#if defined(_WIN32)

#include <winsock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <BaseTsd.h>
#include <io.h>

#pragma comment(lib, "Ws2_32.lib")
typedef SSIZE_T ssize_t;

#else // defined(_WIN32)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#if defined(__FreeBSD__)
#include <netinet/tcp.h>
#endif // defined(__FreeBSD__)

#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#endif // defined(_WIN32)

#if defined(_WIN32)
static LPFN_CONNECTEX ConnectEx = NULL;

static BOOL load_mswsock(void) {
	/* Dummy socket needed for WSAIoctl */
	/* WTF?! */
	SOCKET sock;
	GUID guid = WSAID_CONNECTEX;
	DWORD dwBytes;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == INVALID_SOCKET) {
		return(FALSE);
	}

	if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &ConnectEx, sizeof(ConnectEx), &dwBytes, NULL, NULL) != 0) {
		return(FALSE);
	}

	closesocket(sock);
	return(TRUE);
}
#endif // defined(_WIN32

int
main(int argc, char *argv[])
{
	char buf[1024];
	char *req = "GET /cgi-bin/he HTTP/1.0\r\nUser-agent: tcp_fastopen\r\nConnection: close\r\n\r\n";
	struct sockaddr_storage addr;
	struct sockaddr_in *addr4;
	struct sockaddr_in6 *addr6;
	size_t addr_len;

	ssize_t n;
#if defined(__APPLE__)
	sa_endpoints_t endpoints;
	struct iovec iov;
	size_t len;
#endif
#if defined(__FreeBSD__)
	const int on = 1;
#endif
#if defined(_WIN32)
	SOCKET fd;
	WSADATA wsaData;
	OVERLAPPED ol;
	DWORD bytesSent;
	BOOL ok;
	struct sockaddr_in6 bind_addr;
#else // defined(_WIN32)
	int fd;
#endif // defined(_WIN32)

	if (argc != 3) {
		printf("usage: tcp-fastopen IP PORT\n");
		exit(EXIT_FAILURE);
	}

#if defined(_WIN32)
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup failed\n");
		exit(EXIT_FAILURE);
	}

	if (!load_mswsock()) {
		printf("Error loading mswsock functions: %d\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
#endif

	memset(&addr, 0, sizeof(struct sockaddr_storage));
	addr4 = (struct sockaddr_in*) &addr;
	addr6 = (struct sockaddr_in6*) &addr;

	if (inet_pton(AF_INET, argv[1], &(addr4->sin_addr.s_addr)) == 1) {
		addr_len = sizeof(struct sockaddr_in);

		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(atoi(argv[2]));
#if defined(__APPLE__) || defined(__FreeBSD__)
		addr4->sin_len = sizeof(struct sockaddr_in);
#endif
	} else if (inet_pton(AF_INET6, argv[1], &(addr6->sin6_addr.s6_addr)) == 1) {
		addr_len = sizeof(struct sockaddr_in6);
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(atoi(argv[2]));
#if defined(__APPLE__) || defined(__FreeBSD__)
		addr6->sin6_len = sizeof(struct sockaddr_in6);
#endif
	} else {
		printf("inet_pton failed - please enter valid IPv4/IPv6 address!\n");
		return(EXIT_FAILURE);
	}

	if ((fd = socket(addr.ss_family, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

#if defined(__APPLE__)
	endpoints.sae_srcif = 0;
	endpoints.sae_srcaddr = NULL;
	endpoints.sae_srcaddrlen = 0;
	endpoints.sae_dstaddr = (struct sockaddr *)&addr;
	endpoints.sae_dstaddrlen = addr_len;
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
	if (sendto(fd, req, strlen(req), 0, (const struct sockaddr *)&addr, addr_len) < 0) {
		perror("sendto");
	}
#elif defined(__linux__)
	if (sendto(fd, req, strlen(req), MSG_FASTOPEN, (const struct sockaddr *)&addr, addr_len) < 0) {
		perror("sendto");
	}
#elif defined(_WIN32)
	char optVal = TRUE;
	if (setsockopt(fd, IPPROTO_TCP, TCP_FASTOPEN, &optVal, sizeof(optVal)) != 0) {
		printf("setsockopt failed: %d\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin6_family = AF_INET6;
	bind_addr.sin6_addr = in6addr_any;
	bind_addr.sin6_port = 0;
	//inet_pton(AF_INET, "10.0.1.158", &addr.sin_addr.s_addr);
	//addr.sin_port = htons(1988);
	if (bind(fd, (SOCKADDR*)&bind_addr, sizeof(bind_addr)) != 0) {
		printf("bind failed: %d\n", WSAGetLastError());
		return(EXIT_FAILURE);
	}

	memset(&ol, 0, sizeof(ol));
	ok = ConnectEx(fd, (SOCKADDR*)&addr, sizeof(addr), req, strlen(req), &bytesSent, &ol);

	if (ok) {
		printf("ConnectEx ok - 1\n");
	} else if (WSAGetLastError() == ERROR_IO_PENDING) {
		ok = GetOverlappedResult((HANDLE)fd, &ol, &bytesSent, TRUE);
		printf("ConnectEx pending\n");

		if (!ok) {
			printf("ConnectEx fail at 1: %d\n", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
	} else {
		printf("ConnectEx fail at 2: %d\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

#else
#error "Unsupported platform"
#endif

	do {
		if ((n = recv(fd, buf, sizeof(buf), 0)) < 0) {
			perror("recv");
			exit(EXIT_FAILURE);
		} else {
			if (n > 0) {
#if defined(_WIN32)
				_write(1, buf, n);
#else //defined(_WIN32)
				write(1, buf, n);
#endif
			}
		}
	} while (n > 0);

#if defined(_WIN32)
	if (closesocket(fd) < 0) {
		printf("closesocket() failed\n");
		exit(EXIT_FAILURE);
	}
	WSACleanup();
#else
	if (close(fd) < 0) {
		perror("close");
	}
#endif
	return (EXIT_SUCCESS);
}
