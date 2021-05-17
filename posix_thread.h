#ifndef __h_posix_thread_h__
#define __h_posix_thread_h__

#include <stdbool.h>

#include <signal.h>

#include <pthread.h>


struct PlatfromThread;
struct PlatfromMutex;
struct PlatfromCond;

struct PlatfromThread {
	pthread_t pid;
	pid_t tid;
	pthread_key_t key;
	void*(*func)(void*);
	void *args;
};
struct PlatfromMutex {
	pthread_mutex_t mutex_;
};
struct PlatfromCond {
	pthread_cond_t cond_;
};


void PlatfromThread_default(struct PlatfromThread *thread);
void PlatfromThread_init(struct PlatfromThread *thread, void *(*func)(void *), void *args);
bool PlatfromThread_isEqual(struct PlatfromThread * thread);
void PlatfromThread_join(struct PlatfromThread * thread);
pid_t PlatfromThread_getTid(struct PlatfromThread *thread);
pid_t PlatfromThread_getCurrentTid();

void PlatfromMutex_init(struct PlatfromMutex *mutex);
bool PlatfromMutex_Trylock(struct PlatfromMutex *mutex);
void PlatfromMutex_lock(struct PlatfromMutex *mutex);
void PlatfromMutex_unlock(struct PlatfromMutex *mutex);

void PlatfromCond_init(struct PlatfromCond *cond);
void PlatfromCond_wait(struct PlatfromCond *cond, struct PlatfromMutex *mutex);
void PlatfromCond_wakeup(struct PlatfromCond *cond);




#endif
