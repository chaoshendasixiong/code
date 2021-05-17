#ifndef __h_poll_h__
#define __h_poll_h__
#include <stdint.h>




#define MAX_EVENT 128
#define MAX_POLLFD 0xFFFF
#define HASH_ID(id) (((unsigned)id) % MAX_POLLFD)
#define EV_READ 0x01
#define EV_WRITE 0x02

struct poller ;
struct Poll ;
struct poll_event {
	void *ud;
	int read;
	int write;
	int eof;
	int error;
};
#include "platformPoll.h"

struct poll_fd {
	int id;
	int fd;
	//int type; // to dispatch(accept/read)
	void*(*onRead)(struct Poll *poll, struct poll_fd *pfd);
	void*(*onWrite)(struct Poll *poll, struct poll_fd *pfd);
	int evmask;
};

struct Poll {
	struct poller *poller;
	int evfd;
	struct poll_fd pfds[MAX_POLLFD];
	int nevents;
};


void poll_init(struct Poll  *poll);
struct poll_fd * poll_add(struct Poll  *poll, int fd);
void poll_update(struct Poll  *poll, struct poll_fd *pfd);
int poll_wait(struct Poll  *poll, int timeout);
void poll_del(struct Poll  *poll, struct poll_fd * pfd);
void poll_process(struct Poll  *poll);




#endif
