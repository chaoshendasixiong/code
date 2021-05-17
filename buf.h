#ifndef BUF_H_
#define BUF_H_
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "queue.h"


struct Block {
	//uint32_t ref;
	uint32_t size;
	uint8_t *  data;
};

struct Buffer {
	TAILQ_ENTRY(Buffer) ent;
	uint32_t rpos;
	uint32_t wpos;
	struct Block *buf;
};

struct BufferPool {
	TAILQ_HEAD(bufqhead, Buffer) pool;
	uint32_t used;
};

struct BufferPool* BufferPool_create() ;
int BufferPool_in(struct BufferPool *pool, void *inbuf, int len);
int BufferPool_peek(struct BufferPool *pool, void *buf, uint64_t len);
int BufferPool_out(struct BufferPool *pool, void *buf, uint64_t len);

#endif /* BUF_H_ */
