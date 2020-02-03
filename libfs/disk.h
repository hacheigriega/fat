#ifndef _DISK_H
#define _DISK_H

#include <stddef.h>

/** Size of a disk block in bytes */
#define BLOCK_SIZE 4096

/**
 * block_disk_open - Open virtual disk file
 * @diskname: Name of the virtual disk file
 *
 * Open virtual disk file @diskname. A virtual disk file must be opened before
 * blocks can be read from it with block_read() or written to it with
 * block_write().
 *
 * Return: -1 if @diskname is invalid, if the virtual disk file cannot be opened
 * or is already open. 0 otherwise.
 */
int block_disk_open(const char *diskname);

/**
 * block_disk_close - Close virtual disk file
 *
 * Return: -1 if there was no virtual disk file opened. 0 otherwise.
 */
int block_disk_close(void);

/**
 * block_disk_count - Get disk's block count
 *
 * Return: -1 if there was no virtual disk file opened, otherwise the number of
 * blocks that the currently open disk contains.
 */
int block_disk_count(void);

/**
 * block_write - Write a block to disk
 * @block: Index of the block to write to
 * @buf: Data buffer to write in the block
 *
 * Write the content of buffer @buf (%BLOCK_SIZE bytes) in the virtual disk's
 * block @block.
 *
 * Return: -1 if @block is out of bounds or inaccessible or if the writing
 * operation fails. 0 otherwise.
 */
int block_write(size_t block, const void *buf);

/**
 * block_read - Read a block from disk
 * @block: Index of the block to read from
 * @buf: Data buffer to be filled with content of block
 *
 * Read the content of virtual disk's block @block (%BLOCK_SIZE bytes) into
 * buffer @buf.
 *
 * Return: -1 if @block is out of bounds or inaccessible, or if the reading
 * operation fails. 0 otherwise.
 */
int block_read(size_t block, void *buf);

#endif /* _DISK_H */

