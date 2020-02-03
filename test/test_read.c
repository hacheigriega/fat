#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fs.h>


int main(int argc, char **argv)
{
	char diskname[] = "disk.fs";
	fs_mount(diskname);
	//fs_ls(); //print info
	
	int fd = fs_open("hello.txt");
	char *buf = malloc(1000);
	fs_read(fd, buf, 1000); 
	int totalBytesRead = fs_stat(fd);

	printf("Content of the file:\n");
	printf("%.*s", 1000, buf);

	printf("Num of bytes read: %d\n", totalBytesRead);
 

	return 0;
}
