#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "disk.h"

#define block_error(fmt, ...) \
	fprintf(stderr, "%s: "fmt"\n", __func__, ##__VA_ARGS__)

/* Invalid file descriptor */
#define INVALID_FD -1

/* Disk instance description */
struct disk {
	/* File descriptor */
	int fd;
	/* Block count */
	size_t bcount;
};

/* Currently open virtual disk (invalid by default) */
static struct disk disk = { .fd = INVALID_FD };

int block_disk_open(const char *diskname)
{
	int fd;
	struct stat st;

	if (!diskname) {
		block_error("invalid file diskname");
		return -1;
	}

	if (disk.fd != INVALID_FD) {
		block_error("disk already open");
		return -1;
	}

	if ((fd = open(diskname, O_RDWR, 0644)) < 0) {
		perror("open");
		return -1;
	}

	if (fstat(fd, &st)) {
		perror("fstat");
		return -1;
	}

	/* The disk image's size should be a multiple of the block size */
	if (st.st_size % BLOCK_SIZE != 0) {
		block_error("size '%zu' is not multiple of '%d'",
			    st.st_size, BLOCK_SIZE);
		return -1;
	}

	disk.fd = fd;
	disk.bcount = st.st_size / BLOCK_SIZE;

	return 0;
}

int block_disk_close(void)
{
	if (disk.fd == INVALID_FD) {
		block_error("no disk currently open");
		return -1;
	}

	close(disk.fd);

	disk.fd = INVALID_FD;

	return 0;
}

int block_disk_count(void)
{
	if (disk.fd == INVALID_FD) {
		block_error("no disk currently open");
		return -1;
	}

	return disk.bcount;
}

int block_write(size_t block, const void *buf)
{
	if (disk.fd == INVALID_FD) {
		block_error("no disk currently open");
		return -1;
	}

	if (block >= disk.bcount) {
		block_error("block index out of bounds (%zu/%zu)",
			    block, disk.bcount);
		return -1;
	}

	/* Move to the specified block number */
	if (lseek(disk.fd, block * BLOCK_SIZE, SEEK_SET) < 0) {
		perror("lseek");
		return -1;
	}

	/* Perform the actual write into the disk image */
	if (write(disk.fd, buf, BLOCK_SIZE) < 0) {
		perror("write");
		return -1;
	}

	return 0;
}

int block_read(size_t block, void *buf)
{
	if (disk.fd == INVALID_FD) {
		block_error("no disk currently open");
		return -1;
	}

	if (block >= disk.bcount) {
		block_error("block index out of bounds (%zu/%zu)",
			    block, disk.bcount);
		return -1;
	}

	/* Move to the specified block number */
	if (lseek(disk.fd, block * BLOCK_SIZE, SEEK_SET) < 0) {
		perror("lseek");
		return -1;
	}

	/* Perform the actual read from the disk image */
	if (read(disk.fd, buf, BLOCK_SIZE) < 0) {
		perror("read");
		return -1;
	}

	return 0;
}

