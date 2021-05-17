#ifndef TIMER_H_
#define TIMER_H_
#include "rbtree.h"

#include <stdint.h>
struct Timer ;
struct timer_event{
    void *ud;
    int isSet;
    RBTREE_NODE   node;
    void* (*onEvent)(struct Timer  *t, struct timer_event*);
};

struct Timer {
	void *ud;
	RBTREE rbtree;//实体tree
	RBTREE_NODE sentinel;//哨兵node
	uint64_t current_msec;//UTC毫秒级
};

#define TIMER_INFINITE  (uintptr_t)-1
#define TIMER_LAZY_DELAY 300
//#define abs(value)    (((value) >= 0) ? (value) : - (value))
int timer_init(struct Timer *net_timer);//初始化定时器
int timer_howlong(struct Timer *net_timer);//查找最小timer节点
void timer_update_msec(struct Timer *net_timer);//更新定时器当前时间

void timer_del(struct Timer *t, struct timer_event *ev);
void timer_add(struct Timer *t, struct timer_event *ev, uint32_t millseconds);
void timer_poll(struct Timer  *t);

void timer_sleep(uint32_t millseconds);

#endif /* TIMER_H_ */
