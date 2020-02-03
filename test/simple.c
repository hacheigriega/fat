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
	fs_info(); //print info

	printf("\n\nCreating file1...\n");
	fs_create("file1");
	assert(fs_create("file1") == -1);// making a second file with the same name should fail
	fs_ls();

	fs_delete("file1");
	//fs_ls(); //should be empty

	printf("\n\nCreating file2 and file3...\n");
	fs_create("file2");
	fs_create("file3");
	fs_ls();
	
	assert(fs_open("file2") == 0);
	assert(fs_stat(0) == 0);

	printf("\n\nDeleting file3 and creating files 4,5,&6...\n");
	fs_delete("file3");
	fs_create("file4");
	fs_create("file5");
	fs_create("file6");
	fs_ls();

	assert(fs_open("file3") == -1); //file3 does not exist
	assert(fs_open("file4") == 1); //file3 does not exist
	assert(fs_open("file4") == 2); //file3 does not exist

	assert(fs_close(0) == 0);  
	assert(fs_close(1) == 0);  
	assert(fs_close(2) == 0);  

	for (int i=0; i < 32; i++) {
		assert(fs_open("file4") == i + 3);
	}
	assert(fs_open("file5") == -1); // too many files open

	

	return 0;
}
