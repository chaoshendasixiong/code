#include "buf.h"


int main() {

	struct BufferPool* pool = BufferPool_create() ;
	char buf[1024] = "hello";
	int len = strlen(buf);
	printf("%s\n", buf);
	BufferPool_in(pool, buf, len);
	memset(buf, 0x00, sizeof(buf));
	int outlen = BufferPool_out(pool, buf, 13);
	printf("%d %s\n", outlen, buf);
	memset(buf, 0x00, sizeof(buf));
	outlen = BufferPool_out(pool, buf, 2);
	printf("%d %s\n", outlen, buf);
	return 0;
}
