#ifndef __h_thread_h__
#define __h_thread_h__
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
// -------------------------------------------------------------------------------
// 循环队列
struct Queue {
	int head;
	int tail;
	int cap;
	int len;
	void **data;
};
void Queue_init(struct Queue *q);
int Queue_put(struct Queue *q, void *data);
int Queue_get(struct Queue *q, void **data);
void Queue_expand(struct Queue *q);
int Queue_size(struct Queue *q);
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// 处理消息的循环体
#include "event.h"
#include "lock.h"
struct Loop {
	struct Queue queue;
	struct my_event event;
	struct Lock lock;
	int isExit;
};
void Loop_init(struct Loop *loop);
void* Loop_loop(void *args);

void Loop_exit(struct Loop *loop);
void Loop_wait(struct Loop *loop);
void Loop_awake(struct Loop *loop);
void Loop_awake_all(struct Loop *loop);
void Loop_post(struct Loop *loop, void *data);
// -------------------------------------------------------------------------------

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// 通用线程
#include "posix_thread.h"
struct Thread {
	struct PlatfromThread thread_;
	struct PlatfromMutex mutex_;
	struct PlatfromCond cond_;
	void *(*ThreadFunc)(void*);
	 
	void *args;
	int isInit;
	int isStart;
	int isDone;
};
void Thread_default(struct Thread *thread);
void Thread_init(struct Thread *thread, void*(*func)(void*), void *args);
void Thread_run(struct Thread *thread);
int Thread_isCurrent(struct Thread *thread);
pid_t Thread_getTid(struct Thread *thread);
pid_t Thread_getCurrentTid();
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// Thread with Loop
struct Message {
	uint32_t id;
	void *data;
	void*(*handler)(void*);
};
struct _SendMessage {
	struct ThreadLoop* threadLoop;
	struct Message msg;
	int* ready;
};
struct ThreadLoop {
	struct Loop loop;
	struct Thread thread;
	void*(*callback)(struct ThreadLoop *, void*);
};
void ThreadLoop_init(struct ThreadLoop *threadLoop, void*(*func)(struct ThreadLoop *, void*), void *args);
void ThreadLoop_cleanOther(struct ThreadLoop *sender, struct ThreadLoop *recver);
void ThreadLoop_async(struct ThreadLoop *sender, struct ThreadLoop *recver, struct Message *msg);
void ThreadLoop_sync(struct ThreadLoop *sender, struct ThreadLoop *recver, struct Message *msg);
void *ThreadLoop_loop(struct ThreadLoop *threadLoop);

// -------------------------
// 带server的threadloop
//struct ServerThreadLoop {
//	struct Loop loop;
//	struct Thread thread;
//	struct Poll poll;
//	void*(*callback)(struct ServerThreadLoop *, void*);
//};
//
//void ServerThreadLoop_init(struct ServerThreadLoop *server, void*(*func)(struct ThreadLoop *, void*), void *args);
//void *ServerThreadLoop_loop(struct ThreadLoop *threadLoop);


// -------------------------
// 一个线程池包含一组线程以及一个任务队列 其实也相当于一个loop 拥有submit提交任务的接口
struct ThreadPool {
	struct Loop loop;
	int num;
	struct Thread *thrs;
};
// 默认初始化一个固定大小的线程池
void ThreadPool_init(struct ThreadPool *pool, int num);
void ThreadPool_run(struct ThreadPool *pool);
void ThreadPool_submit(struct ThreadPool *pool, void *task);

#endif
