#include "posix_thread.h"
#include "unistd.h"

#include <sys/syscall.h>
#include <sys/types.h>

#include <stdio.h>

static pid_t _Get_linux_tid() {
    return syscall(SYS_gettid);
}

static void *ThreadStart (void *args) {
    struct PlatfromThread *thread = (struct PlatfromThread *)args;
    sigset_t      set;
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	thread->tid = _Get_linux_tid();
	thread->func(thread->args);
}

void PlatfromThread_default(struct PlatfromThread *thread) {
	thread->tid = _Get_linux_tid();
}

void PlatfromThread_init(struct PlatfromThread *thread, void *(*func)(void *), void *args) {
	thread->func = func;
	thread->args = args;

	pthread_key_create(&thread->key, NULL);
	pthread_setspecific(thread->key, thread);
	pthread_create(&thread->pid, NULL, ThreadStart, thread);
}

bool PlatfromThread_isEqual(struct PlatfromThread * thread) {
    printf("cur tid=%d obj tid=%d\n", _Get_linux_tid(), thread->tid);
    return ( thread->tid == _Get_linux_tid() );
}

void PlatfromThread_join(struct PlatfromThread * thread) {
    pthread_join(thread->pid, NULL);
}

pid_t PlatfromThread_getTid(struct PlatfromThread *thread) {
	return thread->tid;
}

pid_t PlatfromThread_getCurrentTid() {
	return _Get_linux_tid();
}

void PlatfromMutex_init(struct PlatfromMutex *mutex) {
	pthread_mutexattr_t mutex_attribute;
	pthread_mutexattr_init(&mutex_attribute);
	//pthread_mutexattr_settype(&mutex_attribute, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex->mutex_, &mutex_attribute);
	pthread_mutexattr_destroy(&mutex_attribute);
}

bool PlatfromMutex_Trylock(struct PlatfromMutex *mutex) {
	if( pthread_mutex_trylock(&mutex->mutex_) != 0 )
		return false;
	return true;
}

void PlatfromMutex_lock(struct PlatfromMutex *mutex) {
	pthread_mutex_lock(&mutex->mutex_);
}
void PlatfromMutex_unlock(struct PlatfromMutex *mutex) {
	pthread_mutex_unlock(&mutex->mutex_);
}

void PlatfromCond_init(struct PlatfromCond *cond) {
	pthread_condattr_t cond_attr;
	pthread_condattr_init(&cond_attr);
	pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
	pthread_cond_init(&cond->cond_, &cond_attr);
	pthread_condattr_destroy(&cond_attr);
}
void PlatfromCond_wait(struct PlatfromCond *cond, struct PlatfromMutex *mutex) {
	pthread_cond_wait(&cond->cond_, &mutex->mutex_);
}
void PlatfromCond_wakeup(struct PlatfromCond *cond) {
	pthread_cond_signal(&cond->cond_);
}



