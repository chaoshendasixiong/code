#ifndef __h_lock_h__
#define __h_lock_h__

#include <pthread.h>

struct Lock {
	pthread_mutex_t lock;
};

static inline void
Lock_init(struct Lock *lock) {
	pthread_mutex_init(&lock->lock, NULL);
}

static inline void
Lock_lock(struct Lock *lock) {
	pthread_mutex_lock(&lock->lock);
}

static inline int
Lock_trylock(struct Lock *lock) {
	return pthread_mutex_trylock(&lock->lock) == 0;
}

static inline void
Lock_unlock(struct Lock *lock) {
	pthread_mutex_unlock(&lock->lock);
}

static inline void
Lock_destroy(struct Lock *lock) {
	pthread_mutex_destroy(&lock->lock);
}

#endif