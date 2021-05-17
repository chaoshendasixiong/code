#include "event.h"

#include <sys/time.h>

#include <time.h> // -lrt  librt clock_gettime

void my_event_init (struct my_event *self)
{
    pthread_condattr_t cond_attr;
    int rc = pthread_mutex_init (&self->mutex, NULL);
    
    rc = pthread_condattr_init(&cond_attr);
    rc = pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
    rc = pthread_cond_init (&self->cond, &cond_attr);
    pthread_condattr_destroy(&cond_attr);
    self->signaled = 0;
}

void my_event_release (struct my_event *self)
{
    int rc = pthread_cond_destroy (&self->cond);
    rc = pthread_mutex_destroy (&self->mutex);
}

void my_event_awake (struct my_event *self)
{
    int rc = pthread_mutex_lock (&self->mutex);
    self->signaled = 1;
    rc = pthread_cond_signal (&self->cond);
    rc = pthread_mutex_unlock (&self->mutex);
}


void my_event_awake_all (struct my_event *self)
{
    int rc = pthread_mutex_lock (&self->mutex);
    self->signaled = 1;
    rc = pthread_cond_broadcast (&self->cond);
    rc = pthread_mutex_unlock (&self->mutex);
}

int my_event_wait (struct my_event *self, int milliseconds)
{
    int rc = 0;
    struct timespec ts;

    if (milliseconds != FOREVER) {
#if 1
        clock_gettime(CLOCK_MONOTONIC, &ts);
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000;
#endif
        ts.tv_sec += (milliseconds / 1000);
        ts.tv_nsec += (milliseconds % 1000) * 1000000;
        // Handle overflow.
        if (ts.tv_nsec >= 1000000000) {
          ts.tv_sec++;
          ts.tv_nsec -= 1000000000;
        }
    }
    rc = pthread_mutex_lock (&self->mutex);

    if (milliseconds != FOREVER) {
        // For example, the master might call pthread_cond_signal 
        // but it does not set isReadyToLoad to true. It's not just 
        // like that - the system may decide to wake you up without 
        // a real reason. Spurious wakeups don't depend from an "error" 
        // in how the master signals the condition variable, 
        // but from how the system implements condition variables under the hood. 
        // spurious wakeup wait' can return without any 'notify' call
        // so need while
        // 1. 虚假唤醒 os会选择唤醒无需任何原因(可能设计问题 避免错过真正的唤醒)
        // 2. 信号中断 阻塞的系统调用在进程被信号中断后，通常会中止阻塞、直接返回 EINTR 错误
        // 2. 惊群效应 多核多个线程被唤醒争夺任务 必须重新检测条件是否满足
        while (!self->signaled && rc == 0) {
            rc = pthread_cond_timedwait(&self->cond, &self->mutex, &ts);
        }
    } else {
        while (!self->signaled && rc == 0) {
            rc = pthread_cond_wait (&self->cond, &self->mutex);
        }
    }
    self->signaled = 0;
    rc = pthread_mutex_unlock (&self->mutex);

    return 0;
}
