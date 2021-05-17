INC_DIR = ./
OBJ_DIR = ./obj
SRC = \
	rbtree.c \
	timer.c  \
	log.c    \
	event.c  \
	posix_thread.c  \
	thread.c  \
	poll.c

CC = gcc
OBJ = $(patsubst %.c,${OBJ_DIR}/%.o,$(notdir ${SRC}))
CFLAGS = -g -I${INC_DIR}
LD_FLAGS = -lpthread -lrt

${OBJ_DIR}/%.o:./%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(CFLAGS)  

test:${OBJ}
	$(CC) test.c ${OBJ} -o $@  $(CFLAGS) $(LD_FLAGS)
test_kfifo:
	$(CC) test_kfifo.c kfifo.c -o $@  -g
.PHONY:test_buf
test_buf:
	$(CC) test_buf.c buf.c -o $@ -g
clean:
	rm -rf test ./obj/* test_kfifo test_buf