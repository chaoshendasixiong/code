#include "kfifo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct student_info {
	int id;
	int age;
}student_info;

void test_kfifo_static() {
	DECLARE_KFIFO(fifo, student_info*, 4);
	INIT_KFIFO(fifo);
	printf("kfifo_len = %d\n", kfifo_len(&fifo));
	printf("kfifo_is_empty=%d\n", kfifo_is_empty(&fifo));
}

void test_kfifo_dynamic() {
	DECLARE_KFIFO_PTR(fifo, student_info*);
	if(kfifo_initialized(&fifo)) {
		printf("kfifo not init\n");
		int ret = kfifo_alloc(&fifo, 1024);
		printf("kfifo_alloc ret =%d\n", ret);
	}
	printf("%p\n", fifo.kfifo.data);
	printf("kfifo_size = %d\n", kfifo_size(&fifo));
	printf("kfifo_len = %d\n", kfifo_len(&fifo));
	printf("kfifo_is_empty=%d\n", kfifo_is_empty(&fifo));
	printf("kfifo_avail=%d\n", kfifo_avail(&fifo));
	printf("kfifo_usedbytes=%d\n", kfifo_peek_len(&fifo));

}

void test_kifo_init() {
	DEFINE_KFIFO(fifo, student_info*, 4);
	printf("kfifo_len = %d\n", kfifo_len(&fifo));
	printf("kfifo_is_empty=%d\n", kfifo_is_empty(&fifo));
	printf("kfifo_size=%d\n", kfifo_size(&fifo));
}
void test_kfifo_add() {
	DEFINE_KFIFO(fifo, student_info*, 4);
	int i;
	int ret;
	for(i = 0; i < 10; i++) {
		student_info *stu = (student_info *)malloc(sizeof(student_info));
		stu->id = i;
		stu->age = i*10;
		ret = kfifo_put(&fifo, stu);
		printf("ret= %d kfifo_len = %d  avail=%d usedbytes=%d\n",
				ret, kfifo_len(&fifo), kfifo_avail(&fifo),kfifo_peek_len(&fifo));

	}
	for(i = 0; i < 10; i++) {
		student_info  stu;
		ret = kfifo_get(&fifo, stu);
		printf("ret= %d kfifo_len = %d  avail=%d usedbytes=%d\n",
				ret, kfifo_len(&fifo), kfifo_avail(&fifo),kfifo_peek_len(&fifo));

	}
}

int main() {
	//test_kfifo_static();
	test_kfifo_dynamic();
	//test_kifo_init();
	//test_kfifo_add();
	return 0;
}
