#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"
#include "thread.h"
#include "poll.h"
#include "net.h"
#include "timer.h"


void* _run(struct ThreadLoop *threadLoop, void* args) {

	struct Message *msg = args;
	log_info("thread %d receive msg id=%d", Thread_getCurrentTid(), msg->id);
	sleep(5);
}

void *test_(void *args) {
	log_info("thread %d %s test_", Thread_getCurrentTid(), (char*)(args));
	return NULL;
}

struct ThreadLoop default_loop;

void testThreadloop() {
	struct ThreadLoop threadLoop1;
	ThreadLoop_init(&threadLoop1, _run, &threadLoop1);
	Thread_run(&threadLoop1.thread);
	sleep(1);

	struct Message *msg = (struct Message*)malloc(sizeof(*msg));
	msg->data = "test";
	msg->handler = test_;
	msg->id = 0;

	int ready = 0;
	struct _SendMessage *smsg = malloc(sizeof(struct _SendMessage));
	smsg->threadLoop = &default_loop;
	smsg->msg = *msg;
	smsg->ready = &ready;
	Thread_default(&default_loop.thread);
	Loop_init(&default_loop.loop);
	ThreadLoop_sync(&default_loop, &threadLoop1, msg);
	log_info("aaaaaaaaaaaa");
	// default loop
	struct Loop *loop = &default_loop.loop;
	while(!loop->isExit) {
		struct _SendMessage *smsg;
		if(!Queue_get(&loop->queue, (void**)&smsg)) {
			struct Message *msg = &smsg->msg;
			log_info("default get a msg");
			*smsg->ready = 1;
			Loop_awake(&smsg->threadLoop->loop);
		}else {
			Loop_wait(loop);
			log_info("bbbbbbbbbbbbbb");
		}
	}
	getchar();
}

void test_threadPool() {
	struct ThreadPool threadPool;
	ThreadPool_init(&threadPool, 3);
	ThreadPool_run(&threadPool);
	int i = 0;
	for(i = 0; i<10; i++) {
		struct Message *msg = (struct Message*)malloc(sizeof(*msg));
		msg->data = "test";
		msg->handler = test_;
		msg->id = i;
		sleep(1);
		ThreadPool_submit(&threadPool, msg);
	}
}

void* onMessage(struct Poll *poll, struct poll_fd *pfd) {
	char buf[1024];
	memset(buf, 0x00, 1024);
	int n = read(pfd->fd, buf, 1024);
	if(n == 0) {
		log_info("  client %d closed\n", pfd->fd);
		poll_del(poll, pfd);
		close(pfd->fd);
	}else{
		log_info("recv client %d %s\n", pfd->fd, buf);
		write(pfd->fd, buf, strlen(buf));
	}
}

void* test_accept(struct Poll *poll, struct poll_fd *pfd) {
	int connfd = do_accept(pfd->fd);
	log_info("connfd = %d\n", connfd);
	struct poll_fd *client_pfd = poll_add(poll, connfd);
	client_pfd->onRead = onMessage;
}

void test_poll() {
	struct TcpServer s;
	s.AcceptHandler = test_accept;
	TcpServer_init(&s, NULL, 9999);
	TcpServer_start(&s);
}

void* test_timer_event(struct Timer  *t, struct timer_event *ev) {
	printf("aaaaaaaaaaaa\n");

	timer_add(t, ev, 1000);
	return NULL;
}

void test_timer() {
	struct Timer t;
	timer_init(&t);
	struct timer_event ev;
	memset(&ev, 0x00, sizeof(struct timer_event));
	ev.onEvent = test_timer_event;
	timer_add(&t, &ev, 1000);
	while(1) {
		int sleeptime = timer_howlong(&t);
		printf("%d\n", sleeptime);
		timer_sleep(sleeptime );
		timer_poll(&t);
	}
}

void* test_server_run(struct ThreadLoop *threadLoop, void* args) {

	struct Message *msg = args;
	log_info("thread %d receive msg id=%d", Thread_getCurrentTid(), msg->id);
	sleep(5);
}

void test_server() {


//	struct ThreadLoop threadLoop1;
//	ThreadLoop_init(&server.loop, test_server_run, &server.loop);
//	Thread_run(&&server.loop.thread);
//	sleep(1);

}

int main() {
	//testThreadloop();
	//test_threadPool();
	test_poll();
	//test_timer();
	//test_server();
	return 0;
}
