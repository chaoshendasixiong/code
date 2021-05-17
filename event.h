#ifndef _h_my_event_
#define _h_my_event_

#define FOREVER -1

struct my_event;


void my_event_init (struct my_event *self);

void my_event_release (struct my_event *self);

void my_event_awake (struct my_event *self);
void my_event_awake_all (struct my_event *self);

int my_event_wait (struct my_event *self, int milliseconds);

 

#include <pthread.h>

struct my_event {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int signaled;
}; 

#endif
