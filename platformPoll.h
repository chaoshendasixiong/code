#ifndef PLATFORMPOLL_H_
#define PLATFORMPOLL_H_
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <errno.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strerror() */

#define POLLER_MAX_EVENTS 128

struct poller  {
	int epfd;
	struct epoll_event events [POLLER_MAX_EVENTS];
};

static struct poller* _poll_create() {
	int epfd;
#if 1
	if ((epfd = epoll_create1(EPOLL_CLOEXEC)) < 0) {
		return NULL;
	}
#else
	if ((epfd = epoll_create(16)) < 0) {
		return NULL;
	}
	//fcntl(pq->epfd, F_SETFD, FD_CLOEXEC);
#endif
	struct poller *p = (struct poller *)malloc(sizeof(struct poller));
	memset(p, 0x00, sizeof(struct poller));
	p->epfd = epfd;
	return p;
}

static int _event_create() {
	int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if(evtfd < 0) {
	   return -1;
	}
	return evtfd;
}

static int _poll_wait(struct poller* p, int timeout) {
	int nevents;
	while (1) {
		nevents = epoll_wait (p->epfd, p->events, POLLER_MAX_EVENTS, timeout);
		if (nevents == -1 && errno == EINTR)
			continue;
		break;// TODO should abort?
	}
	return nevents;
}

static int _poll_event(struct poller* p, struct poll_event *event, int index) {
	uint32_t mask =  p->events[index].events;
	event->ud = p->events[index].data.ptr;
	event->read =  (mask & EPOLLIN )!= 0;
	event->write =  (mask & EPOLLOUT )!= 0;
	event->error =  (mask & EPOLLERR )!= 0;
	event->eof =  (mask & EPOLLRDHUP )!= 0;
	return 0;
}

static int _poll_update(struct poller* p, int fd, void *ud, int isRead, int isWrite) {
	struct epoll_event ev;
	memset(&ev, 0x00, sizeof(struct epoll_event));
	ev.events = (isRead ? EPOLLIN : 0) | (isWrite ? EPOLLOUT : 0);
	ev.data.ptr = ud;
	if (epoll_ctl(p->epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
		return -1;
	}
	return 0;
}

static int _poll_add(struct poller* p, int fd, void *ud) {
	struct epoll_event ev;
	memset(&ev, 0x00, sizeof(struct epoll_event));
	ev.events = EPOLLIN;
	ev.data.ptr = ud;
	if (epoll_ctl(p->epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		return -1;
	}
	return 0;
}

static void _poll_del(struct poller* p, int fd) {
	epoll_ctl(p->epfd, EPOLL_CTL_DEL, fd , NULL);
}

#endif /* PLATFORMPOLL_H_ */
