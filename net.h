#include "poll.h"

#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>

#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

union sockaddr_all {
	struct sockaddr s;
	struct sockaddr_in v4;
	struct sockaddr_in6 v6;
};

static int getSocketError(int sockfd) {
	int optval;
	socklen_t optlen =  sizeof(optval);

	if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
		return errno;
	} else {
		return optval;
	}
}


static int CheckConnect(int sockfd) {
	struct pollfd fd;
	int ret = 0;
	socklen_t len = 0;
	fd.fd = sockfd;
	fd.events = POLLOUT;
	while ( poll (&fd, 1, -1) == -1 ) {
		if( errno != EINTR ){
			perror("poll");
			return -1;
		}
	}
	ret = getSocketError(sockfd);

	if(ret != 0) {
		fprintf (stderr, "socket %d connect failed: %s\n",
				sockfd, strerror (ret));
		return -1;
	}
	return 0;
}


int IsBlockingError(int e) {
  return (e == EWOULDBLOCK) || (e == EAGAIN) || (e == EINPROGRESS);
}

static int do_write(int fd, char *buf, int size) {
	int n;
	for(;;) {
		int n = write(fd, buf, size);
		if (n < 0) {
			if(errno == EINTR) continue;
			if(errno == EAGAIN) break;
			//if(errno == EPIPE || errno == ECONNRESET) break;
		}
	}
	return n;
}

int do_read(int fd, char *buf, int size) {
	int n;
	for(;;) {
		int n=read(fd, buf, size);
		if (n<0) {
			if(errno == EINTR) continue;
			if(errno == EAGAIN) break;
		}
	}
	return n;
}

void update_time(uint64_t *time) {
	time_t           sec;
	uint64_t       msec;
	struct timeval   tv;
	gettimeofday(&tv, NULL);
	sec = tv.tv_sec;
	msec = tv.tv_usec / 1000;
	*time = (uintptr_t) sec * 1000 + msec;

	uint64_t ui64 = *time;
 	#define NGX_INT32_LEN   (sizeof("-2147483648") - 1)
	#define NGX_INT64_LEN   (sizeof("-9223372036854775808") - 1)
	char         *p, temp[NGX_INT64_LEN + 1];
	memset(temp, 0x00, sizeof(temp));

	printf("current_msec =[%"PRIu64"]\n", ui64);
	uint32_t        ui32;
	p = temp + NGX_INT64_LEN;
	if (ui64 <= (uint64_t) 0xffffffff) {
		ui32 = (uint32_t) ui64;
		do {
			*--p = (u_char) (ui32 % 10 + '0');
		} while (ui32 /= 10);
	} else {
		do {
			*--p = (u_char) (ui64 % 10 + '0');
		} while (ui64 /= 10);
	}
	while (p!=temp) {
		*--p  = ' ';
	}
	printf("current_msec =[%s]\n", temp);
}

int setnonblocking(int fd) {
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

static int do_bind(const char *host, int port, int protocol, int *family) {
	int fd;
	int status;
	int reuse = 1;
	struct addrinfo ai_hints;
	struct addrinfo *ai_list = NULL;
	char portstr[16];
	if (host == NULL || host[0] == 0) {
		host = "0.0.0.0";	// INADDR_ANY
	}
	sprintf(portstr, "%d", port);
	memset(&ai_hints, 0, sizeof(ai_hints));
	ai_hints.ai_family = AF_UNSPEC;
	if (protocol == IPPROTO_TCP) {
		ai_hints.ai_socktype = SOCK_STREAM;
	} else {
		assert(protocol == IPPROTO_UDP);
		ai_hints.ai_socktype = SOCK_DGRAM;
	}
	ai_hints.ai_protocol = protocol;

	status = getaddrinfo(host, portstr, &ai_hints, &ai_list);
	if (status != 0) {
		return -1;
	}
	*family = ai_list->ai_family;
	fd = socket(*family, ai_list->ai_socktype, 0);
	if (fd < 0) {
		goto _failed_fd;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*) &reuse, sizeof(int))
			== -1) {
		goto _failed;
	}
	status = bind(fd, (struct sockaddr*) ai_list->ai_addr, ai_list->ai_addrlen);
	if (status != 0)
		goto _failed;

	freeaddrinfo(ai_list);
	return fd;
	_failed: close(fd);
	_failed_fd: freeaddrinfo(ai_list);
	return -1;
}
static int do_listen(const char *host, int port, int backlog) {
	int family = 0;
	int listen_fd = do_bind(host, port, IPPROTO_TCP, &family);
	if (listen_fd < 0) {
		return -1;
	}
	if (listen(listen_fd, backlog) == -1) {
		close(listen_fd);
		return -1;
	}
	return listen_fd;
}
static int do_accept(int sockfd) {
	while (1) {
		int newfd = accept(sockfd, NULL, NULL);
		if (newfd == -1) {
			if (errno == EINTR)
				continue;
			if (errno == EMFILE || errno == ENFILE) {
				//TODO sleep or accept(fd, 0,0) close a fd(need to alloc before)
			}
			if(errno == EAGAIN) return 0; // do other thing

			if (errno != EAGAIN && errno != ECONNABORTED &&
			errno != EPROTO && errno != EINTR)
				printf("error");

			return -1;
		}else
			return newfd;
	}
}



int do_connect(char *ip, int port, int time) {

	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	setnonblocking(sockfd);
	int ret = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
	if(ret == 0) return sockfd;
	// 碰到EINTR错误 accept、read、write、select、和open可以重启 connect can't
	if (errno == EINPROGRESS /*|| EINTR||EISCONN|EALREADY*/) {
		// TODO add to epoll and sockopt or simple sleep and test
		CheckConnect(sockfd);
	}else {
		log_info("errno=%d %s", strerror(errno));
		return -1;
	}
}

struct TcpServer{
	char *host;
	int port;
	int listenfd;
	struct Poll poll;
	void*(*AcceptHandler)(struct Poll *p, struct poll_fd *pfd);
};
// TcpServer_init(&s, NULL, 9999)
void TcpServer_init(struct TcpServer *server, char *host, int port) {
	int listenfd = do_listen(NULL, 9999, 512 );
	setnonblocking(listenfd);

	poll_init(&server->poll);
	struct poll_fd *pfd = poll_add(&server->poll, listenfd);
	pfd->onRead = server->AcceptHandler;

}

void TcpServer_start(struct TcpServer *server) {
	struct Poll *p = &server->poll;
	while(1) {
		if(poll_wait(p, -1) < 0) break;
		poll_process(p);
		sleep(1);
	}
}
