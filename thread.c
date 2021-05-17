#include "thread.h"

#include "log.h"

// -------------------------------------------------------------------------------
// 循环队列
#define INIT_SIZE 64
void Queue_init(struct Queue *q) {
	q->head = 0;
	q->tail = 0;
	q->cap = INIT_SIZE;
	q->len = 0;
	q->data = (void*)malloc(sizeof(void*)*INIT_SIZE);
}

int Queue_put(struct Queue *q, void *data) {
	while(q->len == q->cap) {
		Queue_expand(q);
		return -1;
	}
	q->data[q->tail] = data;
	q->tail = (q->tail + 1) % q->cap;
	q->len++;
	return 0;
}

int Queue_get(struct Queue *q, void **data) {

	if(q->len == 0) return -1;
	*data = q->data[q->head];
	q->head = (q->head + 1) % q->cap;
	q->len--;
	return 0;
}

void Queue_expand(struct Queue *q) {
	void **expand = (void*)malloc(sizeof(void*)*q->cap*2);
	int i = 0;
	for(i = 0; i < q->cap; i++) {
		expand[i] = q->data[(q->head + i) % q->cap];
	}
	q->head = 0;
	q->tail = q->cap;
	q->cap *= 2;
	free(q->data);
	q->data = expand;
}

int Queue_size(struct Queue *q) {
	return q->len;
}
// -------------------------------------------------------------------------------
void* Loop_loop(void *args) {
	struct Loop *loop = (struct Loop *)args;
	void* tmp = 0;
	while(loop->isExit && Queue_get(&loop->queue, &tmp)) {
		Loop_wait(loop);
	}
}

void Loop_init(struct Loop *loop) {
	Queue_init(&loop->queue);
	my_event_init(&loop->event);
	Lock_init(&loop->lock);
	loop->isExit = 0;
}

void Loop_wait(struct Loop *loop) {
	my_event_wait(&loop->event, -1);
}

void Loop_awake(struct Loop *loop) {
	my_event_awake(&loop->event);
}

void Loop_awake_all(struct Loop *loop) {
	my_event_awake_all(&loop->event);
}

void Loop_exit(struct Loop *loop) {
	loop->isExit = 1;
	Loop_awake(loop);
}

void Loop_post(struct Loop *loop, void *data) {
	Lock_lock(&loop->lock);
	Queue_put(&loop->queue, data);
	Lock_unlock(&loop->lock);
}
// -------------------------------------------------------------------------------





// -------------------------------------------------------------------------------
static void* ThreadLoad(void * args) {
	struct Thread *thread = args;
	PlatfromMutex_lock(&thread->mutex_);
	while(!thread->isStart) {
		PlatfromCond_wait(&thread->cond_, &thread->mutex_);
	}
	PlatfromMutex_unlock(&thread->mutex_);
	if(thread->isStart && (thread->ThreadFunc != NULL) ) {
		thread->ThreadFunc(thread->args);
	}
	PlatfromMutex_lock(&thread->mutex_);
	thread->isDone = 1;
	PlatfromCond_wakeup(&thread->cond_);
	PlatfromMutex_unlock(&thread->mutex_);
}
void Thread_default(struct Thread *thread) {
	PlatfromThread_default(&thread->thread_);
	PlatfromMutex_init(&thread->mutex_);
	PlatfromCond_init(&thread->cond_);
	thread->isInit = 1;
	thread->isStart = 1;
	thread->isDone = 0;
}

void Thread_init(struct Thread *thread, void*(*func)(void*), void *args) {
	memset((void*)thread, 0x00, sizeof(struct Thread));
	thread->ThreadFunc = func;
	thread->args = args;
	//assert(func != NULL);
	PlatfromThread_init(&thread->thread_, ThreadLoad, thread);
	PlatfromMutex_init(&thread->mutex_);
	PlatfromCond_init(&thread->cond_);
	thread->isInit = 1;
	thread->isStart = 0;
	thread->isDone = 0;
}


void Thread_run(struct Thread *thread) {
	PlatfromMutex_lock(&thread->mutex_);
	thread->isStart = 1;
	PlatfromCond_wakeup(&thread->cond_);
	PlatfromMutex_unlock(&thread->mutex_);
};

void Thread_quit(struct Thread *thread) {
	if(!thread->isInit) return;
	PlatfromMutex_lock(&thread->mutex_);
	thread->isStart = 1;
	PlatfromCond_wakeup(&thread->cond_);
	PlatfromMutex_unlock(&thread->mutex_);
};

int Thread_isCurrent(struct Thread *thread) {
	return PlatfromThread_isEqual(&thread->thread_);
}

pid_t Thread_getTid(struct Thread *thread) {
	return PlatfromThread_getTid(&thread->thread_);
}

pid_t Thread_getCurrentTid() {
	return PlatfromThread_getCurrentTid();
}

// -------------------------------------------------------------------------------
// Thread with Loop
static void* ThreadLoopLoad(void * args) {
	struct ThreadLoop *threadLoop = args;
	ThreadLoop_loop(threadLoop);
	return NULL;
}
void *ThreadLoop_loop(struct ThreadLoop *threadLoop) {
	struct Loop *loop = &threadLoop->loop;
	while(!loop->isExit) {
		struct _SendMessage *smsg;
		if(!Queue_get(&loop->queue, (void**)&smsg)) {
			struct Message *msg = &smsg->msg;
			if(threadLoop->callback != NULL) {
				threadLoop->callback(threadLoop, msg);
			}
			*smsg->ready = 1;
			Loop_awake(&smsg->threadLoop->loop);
		}else {
			//my_event_wait(&loop->event, -1);
			Loop_wait(loop);
		}
	}
}
// ThreadLoop_init(&threadLoop->thread, ThreadLoopLoad, threadLoop);
void ThreadLoop_init(struct ThreadLoop *threadLoop, void*(*func)(struct ThreadLoop *, void*), void *args) {
	
	Loop_init(&threadLoop->loop);
	threadLoop->callback = func;
	Thread_init(&threadLoop->thread, ThreadLoopLoad, threadLoop);
}

void ThreadLoop_cleanOther(struct ThreadLoop *sender, struct ThreadLoop *recver) {
	// lock
	struct _SendMessage *smsg;
	struct Loop *loop = &sender->loop;
	while(Queue_get(&loop->queue, (void**)&smsg)) {
		if(smsg->threadLoop == recver) {
			// run
			struct Message *msg = &smsg->msg;
			msg->handler(msg->data);
			*smsg->ready = 1;
			Loop_awake(&recver->loop);
		} else {
			Queue_put(&loop->queue, smsg);
		}
	}
}

void ThreadLoop_async(struct ThreadLoop *sender, struct ThreadLoop *recver, struct Message *msg) {
	struct Loop *recvLoop = &recver->loop;
	int ready = 0;
	struct _SendMessage *smsg = malloc(sizeof(struct _SendMessage));
	smsg->threadLoop = sender;
	smsg->msg = *msg;
	smsg->ready = &ready;
	Loop_post(recvLoop, smsg);
	Loop_awake(recvLoop);
}

void ThreadLoop_sync(struct ThreadLoop *sender, struct ThreadLoop *recver, struct Message *msg) {
	// 步骤1 目标线程的消息循环是否还在处理消息？                                        // 步骤1
	struct Thread *sendThread = &sender->thread;
	struct Thread *recvThread = &recver->thread;
	struct Loop *recvLoop = &recver->loop;
	if (!Thread_isCurrent(sendThread)) {
		return;
	}
	if (recvLoop->isExit)
		return ;
	// 步骤2 创建需要处理的消息 
	
	// 步骤3 若目标线程就是自己，那么直接在此处处理完消息就ok
	if (Thread_isCurrent(recvThread)) {
		msg->handler(msg->data);
		//printf("self do %d\n", *data);
	}
	// 步骤4 断言当前线程是否具有阻塞权限，无阻塞权限 省略	
	
	int ready = 0;
	struct _SendMessage *smsg = malloc(sizeof(struct _SendMessage));
	smsg->threadLoop = sender;
	smsg->msg = *msg;
	smsg->ready = &ready;
	Loop_post(recvLoop, smsg);
	
	// 步骤7 将目标线程从IO处理中唤醒，赶紧处理消息啦~ 
	Loop_awake(recvLoop);
	
	// 步骤8 同步等待消息被处理
	int waited = 0;
	Lock_lock(&recvLoop->lock);
	while (!ready) {
		Lock_unlock(&recvLoop->lock);
		// 对方也可能向我Send了消息，可不能都互相阻塞住了
		// 处理对方可能Send给我的消息。
		ThreadLoop_cleanOther(sender, recver);
		// sender->ReceiveSendsFromThread(loop->_thread);
		// 处理完对方的Send消息后，阻塞等待对方处理完我Send的消息，然后来唤醒我吧
		// 但这儿会有个意外，这就是waited存在的意义了
		Loop_wait(&sender->loop);
		waited = 1;
		Lock_lock(&recvLoop->lock);
	}
	Lock_unlock(&recvLoop->lock);

	// 步骤9 如果出现过waited，那么再唤醒一次当前线程去处理Post消息。
	if (waited) {
		Loop_awake(&sender->loop);
	}
}

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------

static void* poll_handler(void *args) {
	struct ThreadPool *pool = args;
	struct Loop *loop = &pool->loop;
	while(!loop->isExit) {
		struct Message *msg;
		if(!Queue_get(&loop->queue, (void**)&msg)) {
			log_info("thread %d receive msg id=%d", Thread_getCurrentTid(), msg->id);
			if(msg->data != NULL)
				log_info("thread %d receive msg id=%s", Thread_getCurrentTid(), msg->data);
			//msg->handler(msg->data);
		}else {
			//my_event_wait(&loop->event, -1);
			Loop_wait(loop);
		}
	}
}

void ThreadPool_init(struct ThreadPool *pool, int num) {
	Loop_init(&pool->loop);
	pool->num = num;
	pool->thrs = malloc(sizeof(struct Thread)*num);
	int i = 0;
	for(i = 0; i < num; i++) {
		Thread_init(&pool->thrs[i], poll_handler, pool);
	}
}
void ThreadPool_run(struct ThreadPool *pool) {
	int i = 0;
	for(i = 0; i < pool->num; i++) {
		Thread_run(&pool->thrs[i]);
	}
}

void ThreadPool_submit(struct ThreadPool *pool, void *task) {
	Loop_post(&pool->loop, task);
	Loop_awake_all(&pool->loop);
}
