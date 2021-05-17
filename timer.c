#include "timer.h"
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

int timer_init(struct Timer *t)
{
    rbtree_init(&t->rbtree, &t->sentinel, rbtree_insert_timer_value);
    timer_update_msec(t);
    return 0;
}

struct timer_event * timer_node(struct Timer *t) {
	struct timer_event *ev;
	void *ret;
	RBTREE_NODE  *node, *root, *sentinel;
	if (t->rbtree.root == &t->sentinel) {
		return NULL;//-1表示空树
	}
	root = t->rbtree.root;
	sentinel = t->rbtree.sentinel;
	node = rbtree_min(root, sentinel);
	ret = ((char *) node - offsetof(struct timer_event, node));
	ev = (struct timer_event *)ret;
	return  ev;
}

int timer_howlong(struct Timer *t)//找到最小时间节点 返回当前差值
{
    int timer;
    struct timer_event *ev = timer_node(t);
    timer = ev->node.key-t->current_msec;
    return (timer > 0 ? timer : 0);
}

void timer_update_msec(struct Timer *t)
{
    time_t           sec;
    time_t           msec;
    struct timeval   tv;
    gettimeofday(&tv, NULL);
    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;
    //uintptr_t 保证在32位和64位精度保持一致ms级
    //1510020810000 1510020815474
    //2487289576    2487294578
    t->current_msec = (uintptr_t)sec * 1000 + msec;
    //update_time(&t->current_msec);
}

void timer_del(struct Timer *t, struct timer_event *ev)
{//删除一个事件绑定的定时器 并归零timer_set
    rbtree_delete(&t->rbtree, &ev->node);
    ev->isSet = 0;
}
void timer_add(struct Timer *t, struct timer_event *ev, uint32_t millseconds)
{//增加一个事件绑定的定时器
    int  key;
    int  diff;//时间差值
    key = t->current_msec + millseconds;
    if (ev->isSet) {//如果该事件已经设置过 判断差值
        diff =  (key - ev->node.key);
        if (abs(diff) < TIMER_LAZY_DELAY) {//小于一定的间隔300ms 认为执行过 等待下次
            return;
        }
        timer_del(t, ev);//删除该定时node 重新插入
    }
    ev->node.key = key;
    rbtree_insert(&t->rbtree, &ev->node);//插入到rbtree
    ev->isSet = 1;//标志位设1
}

void timer_poll(struct Timer  *t)//处理timer到的事件
{
	timer_update_msec(t);//先更新时间
	struct timer_event *ev = timer_node(t);
	if(ev == NULL) return;
	//最小timer大于当前时间 不做任何事
	int delta = ev->node.key-t->current_msec;
	if ( delta > 0) return;
	timer_del(t, ev);
	ev->onEvent(t, ev);//执行回调函数
}
void timer_sleep(uint32_t millseconds) {
	struct timeval delay;
	delay.tv_sec = millseconds/1000;
	delay.tv_usec = (millseconds%1000) * 1000; // 20 ms
	select(0, NULL, NULL, NULL, &delay);
}




