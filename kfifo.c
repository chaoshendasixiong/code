
#include "kfifo.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline
int is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

static inline uint32_t
roundup_power_of_2(uint32_t x)
{
    if (x == 0) return 0;
	if((x & (x-1)) == 0) return x;

    uint32_t position = 0;
    for (int i = x; i != 0; i >>= 1)
        position++;

    return (uint32_t)(1 << position);
}

uint32_t roundup_power_of_two(uint32_t x)
{
	if (x == 0) return 0;
	if((x & (x-1)) == 0) return x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

uint32_t roundup_power_of_two_bak(uint32_t val)
{
    if((val & (val-1)) == 0)
        return val;

    uint32_t max = (uint32_t)((uint32_t)~0);
    uint32_t andv = ~(max&(max>>1));
    while((andv & val) == 0)
        andv = andv>>1;
    return andv<<1;
}

static inline unsigned int kfifo_unused(struct __kfifo *fifo) {
	return (fifo->mask + 1) - (fifo->in - fifo->out);
}

int __kfifo_alloc(struct __kfifo *fifo, unsigned int size, size_t esize) {

	size = roundup_power_of_two(size);

	fifo->in = 0;
	fifo->out = 0;
	fifo->esize = esize;

	if (size < 2) {
		fifo->data = NULL;
		fifo->mask = 0;
		return -EINVAL;
	}

	fifo->data = malloc(esize*size);

	if (!fifo->data) {
		fifo->mask = 0;
		return -ENOMEM;
	}
	fifo->mask = size - 1;
	return 0;
}

void __kfifo_free(struct __kfifo *fifo) {
	free(fifo->data);
	fifo->in = 0;
	fifo->out = 0;
	fifo->esize = 0;
	fifo->data = NULL;
	fifo->mask = 0;
}

int __kfifo_init(struct __kfifo *fifo, void *buffer,
		unsigned int size, size_t esize) {
	size /= esize;

	if (!is_power_of_2(size))
		size = rounddown_pow_of_two(size);

	fifo->in = 0;
	fifo->out = 0;
	fifo->esize = esize;
	fifo->data = buffer;

	if (size < 2) {
		fifo->mask = 0;
		return -EINVAL;
	}
	fifo->mask = size - 1;
	return 0;
}

static void kfifo_copy_in(struct __kfifo *fifo, const void *src,
		unsigned int len, unsigned int off) {
	unsigned int size = fifo->mask + 1;
	unsigned int esize = fifo->esize;
	unsigned int l;

	off &= fifo->mask;
	if (esize != 1) {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);

	memcpy(fifo->data + off, src, l);
	memcpy(fifo->data, src + l, len - l);
}

unsigned int __kfifo_in(struct __kfifo *fifo, const void *buf, unsigned int len) {
	unsigned int l;

	l = kfifo_unused(fifo);
	if (len > l)
		len = l;

	kfifo_copy_in(fifo, buf, len, fifo->in);
	fifo->in += len;
	return len;
}

static void kfifo_copy_out(struct __kfifo *fifo, void *dst, unsigned int len,
		unsigned int off) {
	unsigned int size = fifo->mask + 1;
	unsigned int esize = fifo->esize;
	unsigned int l;

	off &= fifo->mask;
	if (esize != 1) {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);

	memcpy(dst, fifo->data + off, l);
	memcpy(dst + l, fifo->data, len - l);
}

unsigned int __kfifo_out_peek(struct __kfifo *fifo, void *buf, unsigned int len) {
	unsigned int l;

	l = fifo->in - fifo->out;
	if (len > l)
		len = l;

	kfifo_copy_out(fifo, buf, len, fifo->out);
	return len;
}

unsigned int __kfifo_out(struct __kfifo *fifo, void *buf, unsigned int len) {
	len = __kfifo_out_peek(fifo, buf, len);
	fifo->out += len;
	return len;
}


unsigned int __kfifo_max_r(unsigned int len, size_t recsize) {
	unsigned int max = (1 << (recsize << 3)) - 1;

	if (len > max)
		return max;
	return len;
}

static unsigned int __kfifo_peek_n(struct __kfifo *fifo, size_t recsize) {
	unsigned int l;
	unsigned int mask = fifo->mask;
	unsigned char *data = fifo->data;

	l = ((data)[(fifo->out) & (mask)]);

	if (--recsize)
		l |= ((data)[(fifo->out + 1) & (mask)]) << 8;

	return l;
}

static void __kfifo_poke_n(struct __kfifo *fifo, unsigned int n, size_t recsize) {
	unsigned int mask = fifo->mask;
	unsigned char *data = fifo->data;

	((data)[(fifo->in) & (mask)] = (unsigned char) (n));

	if (recsize > 1)
		((data)[(fifo->in + 1) & (mask)] = (unsigned char) (n >> 8));
}

unsigned int __kfifo_len_r(struct __kfifo *fifo, size_t recsize) {
	return __kfifo_peek_n(fifo, recsize);
}

unsigned int __kfifo_in_r(struct __kfifo *fifo, const void *buf,
		unsigned int len, size_t recsize) {
	if (len + recsize > kfifo_unused(fifo))
		return 0;

	__kfifo_poke_n(fifo, len, recsize);

	kfifo_copy_in(fifo, buf, len, fifo->in + recsize);
	fifo->in += len + recsize;
	return len;
}

static unsigned int kfifo_out_copy_r(struct __kfifo *fifo, void *buf,
		unsigned int len, size_t recsize, unsigned int *n) {
	*n = __kfifo_peek_n(fifo, recsize);

	if (len > *n)
		len = *n;

	kfifo_copy_out(fifo, buf, len, fifo->out + recsize);
	return len;
}

unsigned int __kfifo_out_peek_r(struct __kfifo *fifo, void *buf,
		unsigned int len, size_t recsize) {
	unsigned int n;

	if (fifo->in == fifo->out)
		return 0;

	return kfifo_out_copy_r(fifo, buf, len, recsize, &n);
}

unsigned int __kfifo_out_r(struct __kfifo *fifo, void *buf, unsigned int len,
		size_t recsize) {
	unsigned int n;

	if (fifo->in == fifo->out)
		return 0;

	len = kfifo_out_copy_r(fifo, buf, len, recsize, &n);
	fifo->out += n + recsize;
	return len;
}

void __kfifo_skip_r(struct __kfifo *fifo, size_t recsize) {
	unsigned int n;

	n = __kfifo_peek_n(fifo, recsize);
	fifo->out += n + recsize;
}
