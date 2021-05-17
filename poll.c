#include "poll.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "platformPoll.h"

static void* onWake(struct Poll *poll, struct poll_fd *pfd) {
	return NULL;
}

void poll_init(struct Poll  *poll) {
	memset(poll, 0x00, sizeof(struct Poll));
	poll->poller = _poll_create();
	poll->evfd = _event_create();
	struct poll_fd *new_pfd = poll_add(poll, poll->evfd);
	new_pfd->onRead =onWake;
	poll_update(poll, new_pfd);
}

struct poll_fd * poll_add(struct Poll  *poll, int fd) {
	int index = HASH_ID(fd);
	struct poll_fd *pfd = &poll->pfds[index];
	pfd->id = index;
	pfd->fd = fd;
	pfd->evmask |= EV_READ;
	_poll_add(poll->poller, fd, pfd);
	return pfd;
}

void poll_del(struct Poll  *poll, struct poll_fd * pfd) {
	_poll_del(poll->poller, pfd->fd);
}

void poll_update(struct Poll  *poll, struct poll_fd *pfd) {
	_poll_update(poll->poller, pfd->fd, pfd, pfd->evmask&EV_READ, pfd->evmask&EV_WRITE);
}


int poll_wait(struct Poll  *poll, int timeout){
	poll->nevents = 0;
	int nread =_poll_wait(poll->poller, timeout);
	poll->nevents = nread;
	if(nread < 0) return nread;
}

void poll_process(struct Poll  *poll) {
	int i;
	// TODO expired timers
	struct poll_event event;
	for(i = 0; i < poll->nevents; i++) {
		memset(&event, 0x00, sizeof(struct poll_event));
		_poll_event(poll->poller, &event, i);
		struct poll_fd *pfd = event.ud;
		if(event.read && pfd->onRead) pfd->onRead(poll, pfd);
		if(event.write && pfd->onWrite) pfd->onWrite(poll, pfd);
	}
}


