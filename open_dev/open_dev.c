#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>

int main(void)
{
	int             fd;
	char           *mptr;
//	fd = open("/dev/pmem0", O_RDWR | O_SYNC);
	fd = open("/dev/dax0.0", O_RDWR | O_SYNC);
	if (fd == -1) {
		printf("open error...\n");
		return 1;
	}
	mptr = mmap(0, 1 * 1024 * 1024, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd, 4096);
	close(fd);
	printf("On start, mptr points to 0x%lX.\n",(unsigned long) mptr);
	printf("mptr points to 0x%lX. *mptr = 0x%X\n", (unsigned long) mptr, *mptr);
	mptr[0] = 'a';
	mptr[1] = 'b';
	printf("mptr points to 0x%lX. *mptr = 0x%X\n", (unsigned long) mptr, *mptr);
	printf("in dev, %c %c\n", mptr[0], mptr[1]);

	return 0;
}
