#include "buf.h"

#define BLOCK_SIZE 512

static struct Block*
Block_new(uint32_t size) {
	struct Block *result;
	int to_alloc = BLOCK_SIZE;
	while(to_alloc < size) {
		to_alloc <<= 1;
	}
	result = malloc(sizeof(struct Block)+to_alloc);
	result->size = to_alloc;
	result->data = (uint8_t*)(result+1);
	if (result == NULL)
		return NULL;
	return result;
}

static struct Buffer*
BufferPool_expand(struct BufferPool *q, uint32_t bufSize) {
	struct Buffer *ent;
	ent = malloc(sizeof(struct Buffer));
	if (ent == NULL) return NULL;
	ent->buf = Block_new(bufSize);
	if (ent->buf == NULL) {
		free(ent);
		return NULL;
	}
	TAILQ_INSERT_TAIL(&q->pool, ent, ent);
	return ent;
}

struct BufferPool* BufferPool_create() {
	struct BufferPool *ret;
	ret = malloc(sizeof(struct BufferPool));
	if (ret == NULL)
		return NULL;
	TAILQ_INIT(&ret->pool);
	ret->used = 0;
	if (!(BufferPool_expand(ret, 0))) {
		free(ret);
		return NULL;
	}
	return ret;
}

static int
Buffer_in(struct Buffer*buffer, void *buf, int len) {

	int avail = buffer->buf->size - buffer->wpos;
	if(avail >= len) {
		memmove(buffer->buf->data + buffer->wpos, buf, len);
		buffer->wpos += len;
		return len;
	}else {
		memmove(buffer->buf->data + buffer->wpos, buf, len-avail);
		buffer->wpos += len-avail;
		return len-avail;
	}

}

int BufferPool_in(struct BufferPool *pool, void *inbuf, int len) {
	struct Buffer *last, *append;
 	uint32_t copylen = 0;
 	uint32_t tmp = 0;

 	if(TAILQ_EMPTY(&pool->pool))
 		last = BufferPool_expand(pool, len);
 	else
 		last = TAILQ_LAST(&pool->pool, bufqhead);
	copylen = Buffer_in(last, inbuf, len);
	if(copylen < len) {
		append = BufferPool_expand(pool, len);
		if(append) {
			tmp = Buffer_in(append, inbuf, len - copylen);
			copylen += tmp;
		}
	}
	pool->used+=copylen;
	return copylen;
}

static int
BufferPool_remove(struct BufferPool *pool, void *buf,uint64_t len, int isdel) {
	struct Buffer *ent, *tmp;
	uint32_t copy_sum = 0;
	uint32_t avail = 0;
	TAILQ_FOREACH_SAFE(ent, &pool->pool, ent, tmp) {
		avail = ent->wpos - ent->rpos;
		avail = ((avail <= len)? avail:len);
		memcpy(buf, ent->buf->data+ent->rpos, avail);
		buf += avail;
		len -= avail;
		copy_sum += avail;
		if(isdel) {
			ent->rpos += avail;
			if(ent->wpos>= ent->buf->size && ent->wpos == ent->rpos) {
				free(ent->buf);
				free(ent);
				TAILQ_REMOVE(&pool->pool, ent, ent);
			}
		}
		if(len <= 0) break;
	}
	return copy_sum;
}

int BufferPool_peek(struct BufferPool *pool, void *buf, uint64_t len) {
	return BufferPool_remove(pool, buf, len, 0);
}

int BufferPool_out(struct BufferPool *pool, void *buf, uint64_t len) {
	return BufferPool_remove(pool, buf, len, 1);
}

