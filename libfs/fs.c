#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "disk.h"
#include "fs.h"

/*FAT end-of-chain value*/
#define FAT_EOC 0xFFFF 

typedef struct __attribute__((__packed__)) Superblock {
	uint8_t sig[8];
	uint16_t numBlocks;
	uint16_t rootIndex;
	uint16_t dataIndex;
	uint16_t numDataBlocks;
	uint8_t numFAT; // number of blocks for FAT
	uint8_t padding[4079];
}Superblock;

typedef struct __attribute__((__packed__)) FAT {
	uint16_t content;
}FAT;

typedef struct __attribute__((__packed__)) Root {
	uint8_t name[16]; 
	uint32_t size; //file size in bytes
	uint16_t indexFirstBlock;
	uint8_t padding[10]; 
}Root;

typedef struct FD{
	int id;
	int offset;
	int index; // index of the file of the root directory 

}FD;



/* internal data structs for metadata */
static Superblock* sblk;// Global instance
static FAT* fat;// num of entries should equal num_data_blocks
static Root root[128]; // Global instance

static FD filedes[FS_OPEN_MAX_COUNT];
static int numFilesOpen = 0;
static int idCount = 0; // running count of ids to assign 



/* returns the number of empty data blocks */
int num_free_fat() {
	int count = 0;
	for (int i=1; i < sblk->numDataBlocks; i++) { // first data block cannot be used
		if(fat[i].content == 0) // if entry is empty 
			count++;
	}
	return count;
}

/* returns the number of root directory entries that are empty */
int num_free_rdir() {
	int count = 0;
	for (int i=0; i < FS_FILE_MAX_COUNT; i++) {
		if((char)*(root[i].name) == '\0') //if entry is empty
			count++;
	}
	return count;
}

int fs_info()
{
	// Check the presence of an underlying virtual disk
	if (block_disk_count() == -1)
		return -1;

	// Printing information
	printf("FS Info:\n");
	printf("total_blk_count=%d\n", sblk->numBlocks);
	printf("fat_blk_count=%d\n", sblk->numFAT);
	printf("rdir_blk=%d\n", sblk->rootIndex);
	printf("data_blk=%d\n", sblk->dataIndex);
	printf("data_blk_count=%d\n", sblk->numDataBlocks);

	printf("fat_free_ratio=%d/%d\n", num_free_fat(), sblk->numDataBlocks);
	printf("rdir_free_ratio=%d/128\n", num_free_rdir());
	

	return 0;
}


/*
 * sb_init 
 * Extract the file system information into internal (global) data struct
 * Also perform error check
 * Return -1 if error is found. 0 otherwise.
 */
int sb_init() {
	// Reading superblock (first block of the file system)
	sblk = malloc(BLOCK_SIZE);
	block_read(0, sblk);
	
	// Check if signature is as expected
	if (strncmp((char*)(sblk->sig), "ECS150FS", 8) != 0) {
		fprintf(stderr, "Incorrect signature\n");
		return -1;
	}

	// Check if total amount of blocks is correct
	if (sblk->numBlocks != block_disk_count()) {
		fprintf(stderr, "Incorrect total number of blocks\n");
		return -1;
	}
	
	// Check if number of blocks for FAT is correct 
	// uint8_t trueNumFAT = block_disk_count()*2.0/BLOCK_SIZE;
	// printf("true %d, actual: %d \n", trueNumFAT, sblk->numFAT);
	// if (sblk->numFAT != trueNumFAT) {
	// 	fprintf(stderr, "Incorrect number of blocks for FAT\n");
	// 	return -1;
	// }

	// Check if root index is correct
	if (sblk->rootIndex != (sblk->numFAT + 1)) {
		fprintf(stderr, "Incorrect root index\n");
		return -1;
	}

	// Check if data index is correct
	if (sblk->dataIndex != (sblk->rootIndex + 1)) {
		fprintf(stderr, "Incorrect data index\n");
		return -1;
	}

	return 0; // No error found
}

/* read in FAT */
int fat_init() {
	fat = malloc((sblk->numFAT)*BLOCK_SIZE); //get size of FAT array
	int i;
	for(i = 1; i <= sblk->numFAT; i++) {
		block_read(i, (char*)fat + BLOCK_SIZE*(i-1));
	}
	

	if(fat[0].content != FAT_EOC) // first entry is always invalid
		return -1;

	return 0; 
}

/* read in root */
int root_init() {
	block_read(sblk->rootIndex, root);
	return 0; // all good
}


int fd_init() {
	for(int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
		filedes[i].id = -1;
		filedes[i].offset = 0;
		filedes[i].index = -1;
	}
	return 0;
}
	
int fs_mount(const char *diskname)
{
	if (block_disk_open(diskname) != 0)
		return -1; // Open failed

	// Read in metadata in order
	if (sb_init() != 0) // 1. superblock 
		return -1; // Error checking failed
	fat_init(); // 2. FAT
	root_init(); // 3. root directory

	// initialize file descriptors
	fd_init();

	return 0;
}

int fs_umount(void)
{
	// Write back to disk the meta-information
	block_write(0, sblk);

	int i;
	for(i = 1; i <= sblk->numFAT; i++) {
		block_write(i, (char*)fat + BLOCK_SIZE*(i-1));
	} // fat

	// root directory
	block_write(sblk->rootIndex, root);

	if (block_disk_close() != 0)
		return -1; // Close failed

	return 0;
}

/* helper function to check valid filename */
int valid_filename(const char *filename){
	char bad_chars[] = "!@%^*~|";
	for (int i = 0; i < strlen(bad_chars); i++) {
		if(strchr(filename, bad_chars[i]) != NULL)
			return -1;
	} //check valid filename
	
	return 0;
}

int find_empty_fat(){
	uint16_t itr = 1;
	while(itr < sblk->numDataBlocks){
		if(fat[itr].content == 0) {
			return itr;
		}		
		itr++;
	}

	return -1; // no space

}

int fs_create(const char *filename)
{
	if(strlen(filename)*sizeof(char) > FS_FILENAME_LEN) 
		return -1; //filename too long

	if(valid_filename(filename) == -1)
		return -1;

	for (int j = 0; j < FS_FILE_MAX_COUNT; j++){
		if(strncmp((char*)root[j].name, filename, strlen(filename)) == 0) // filename already exists;
			return -1; 	
	}

	int k;
	for(k = 0; k < FS_FILE_MAX_COUNT; k++) {
		if((char) *(root[k].name) == '\0') { //empty entry 
			strcpy((char*) root[k].name, filename);
			root[k].size = 0; 
			root[k].indexFirstBlock = find_empty_fat();
			fat[root[k].indexFirstBlock].content = FAT_EOC;
			break;
		}
	}

	if(k == FS_FILE_MAX_COUNT) // no empty entries
		return -1;

	return 0;
}

int fs_delete(const char *filename)
{
	if(valid_filename(filename) == -1)
		return -1;

	int j;
	for (j = 0; j < FS_FILE_MAX_COUNT; j++){
		if(strncmp((char*)root[j].name, filename, strlen(filename)) == 0){ // found the file
			*(root[j].name) = (int) '\0'; // just clear the name
			uint16_t itr = root[j].indexFirstBlock; //to go through the FAT
			while(itr != FAT_EOC){
				uint16_t itr2 = fat[itr].content;
				fat[itr].content = 0; 
				itr = itr2;
			} //clear FAT blocks
			break;
		}
	}

	if(j == FS_FILE_MAX_COUNT)
		return -1; //file not found
	
	return 0;
}

int fs_ls(void)
{
	//check underlying virtual disk
	if (block_disk_count() == -1)
		return -1;

	printf("FS Ls:\n");
	int i;
	for (i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if((char)*(root[i].name) != '\0') //if name not empty
			printf("file: %s, size: %d, data_blk: %d\n", root[i].name, root[i].size, root[i].indexFirstBlock); //print it out
	}

	return 0;
}

int fs_open(const char *filename)
{
	if(valid_filename(filename) == -1)
		return -1;

	if (numFilesOpen > FS_OPEN_MAX_COUNT)
		return -1; //too many files open
	
	int j, k; // for loop iteration
	for (j = 0; j < FS_FILE_MAX_COUNT; j++) { // iterate through files in root directory
		
		if(strncmp((char*)root[j].name, filename, strlen(filename)) == 0){ // found the file
			
			// iterate through file descriptors to find empty entry
			for(k = 0; k < FS_OPEN_MAX_COUNT; k++){ 
				
				if(filedes[k].id == -1) { // if empty

					// initialize variables
					filedes[k].id = idCount;
					filedes[k].index = j;
					filedes[k].offset = 0;

					idCount++;
					numFilesOpen++;
					
					return filedes[k].id;
				}	
			}
		}
	} 
	return -1; //file not found	
}


int fs_close(int fd)
{
	if (fd > idCount || fd < 0)
		return -1; //invalid fd

	int i;
	for(i = 0; i < FS_OPEN_MAX_COUNT; i++) {
		if(filedes[i].id == fd){ //found fd
			filedes[i].id = -1;
			filedes[i].offset = 0;
			filedes[i].index = -1;
			numFilesOpen--;
			return 0;
		}
	}
	return -1; // file not found	
}

int fs_stat(int fd)
{
	if (fd > idCount || fd < 0)
		return -1; //invalid fd

	int i;
	for (i = 0; i < FS_OPEN_MAX_COUNT; i++){
		if(filedes[i].id == fd) //found fd
			return root[filedes[i].index].size;
	}
	return -1; // fd not found
}

int fs_lseek(int fd, size_t offset)
{
	int size = fs_stat(fd);
	if(size == -1 || offset > size)
		return -1;

	int i;
	for(i = 0; i < FS_OPEN_MAX_COUNT; i++) {
		if(filedes[i].id == fd){ //found fd
			filedes[i].offset = offset;
			return 0;
		}
	}
	return -1; // file not found	
}


/* Return the index of the entry in filedes corresponding to fd */
int filedes_index(int fd)
{
	if (fd > idCount || fd < 0)
		return -1; //invalid fd

	for(int i = 0; i < FS_OPEN_MAX_COUNT; i++) {
		if(filedes[i].id == fd){ //found fd
			return i;	
		}
	}
	return -1; // fd not found
}

/* 
 * Returns the index of the data block corresponding to the file’s offset 
 * Returns -1 if a new block should be allocated
 */
int dataBlk_index(int fd) 
{
	int fdInd = filedes_index(fd); // get filedes index 
	int rootInd = filedes[fdInd].index; // get root dir index
	uint16_t dataInd = root[rootInd].indexFirstBlock; // first data block index

	int offset = filedes[fd].offset; 
	while(offset >= BLOCK_SIZE) { // if offset goes over current block
		// go to next block
		dataInd = fat[dataInd].content;
		if (dataInd == FAT_EOC) // if next index has not been allocated
			return -1;
		offset = offset - BLOCK_SIZE;
	}

	return dataInd + sblk->dataIndex;
} 

/* Change the size of the file corresponding to the specified file descriptor */
void set_file_size(int fd, int sizeInc) {
	int fdInd = filedes_index(fd); // get filedes index 
	int rootInd = filedes[fdInd].index; // get root dir index
	root[rootInd].size += sizeInc;
}


int fs_read(int fd, void *buf, size_t count)
{	
	int fdInd = filedes_index(fd);
	if (fdInd == -1)
		return -1; // fd invalid or not found

	int bufOff = 0; // offset for read buffer
	size_t toRead = count; // remaining # of bytes to (try to) read
	size_t leftOff, rightOff, bytesRead; // left & right offsets, # of bytes read

	int i = 0; // loop iteration count 
	while(toRead != 0) { // more bytes to read 
		
		// Set left & right offsets
		// Left offset may be nonzero only if it is first block read
		leftOff = 0;
		if (i == 0)
			leftOff = filedes[fdInd].offset % BLOCK_SIZE;
		// Right offset is non-zero if we don't need to read another block
		rightOff = 0;
		if (toRead + leftOff < BLOCK_SIZE) {
			rightOff = BLOCK_SIZE - toRead - leftOff;
		}
		
		// Read whole block into bounce buffer
		void *bBuf = malloc(BLOCK_SIZE);
		if (block_read(dataBlk_index(fd), bBuf) == -1) {
			fprintf(stderr, "Error in block reading");
			return count - toRead; // return the number of bytes sucessfully read
		}

		// Copy into read buffer, with offsets in mind
		bytesRead = BLOCK_SIZE - leftOff - rightOff;
		memcpy((char*)buf+bufOff, (char*)bBuf+leftOff, bytesRead); // (dest, src, length in bytes)
		
		// Update status variables accordingly
		filedes[fd].offset = filedes[fd].offset + bytesRead; // update file offset
		bufOff = bufOff + bytesRead; // update read buffer offset
		toRead = toRead - bytesRead; // update # of bytes to read
		free(bBuf); 
		i++;
	}

	return count - toRead; // return the number of bytes sucessfully read
}

void allocate_block(int fd){

	int fdInd = filedes_index(fd);
	int rootInd = filedes[fdInd].index;
	uint16_t dataInd = root[rootInd].indexFirstBlock;
	uint16_t itr = dataInd; //to go through the FAT
	while(fat[itr].content != FAT_EOC){
		itr = fat[itr].content;
	} 
	
	int newInd;
	if ((newInd = find_empty_fat()) == -1) { // if full, do nothing
		return;
	} else {
		fat[itr].content = newInd;
		fat[fat[itr].content].content = FAT_EOC;
	}
	//printf("Allocated a new block: %d\n", fat[itr].content);
}

int fs_write(int fd, void *buf, size_t count)
{
	int fdInd = filedes_index(fd);
	if (fdInd == -1)
		return -1; // fd invalid or not found	

	int bufOff = 0; // offset for buffer containing content to write
	size_t toWrite = count; // remaining # of bytes to (try to) write
	size_t leftOff, rightOff, bWritten; // left & right offsets, # of bytes written in the iteration

	int i = 0; // loop iteration count 
	while(toWrite != 0) { // more bytes to read 
		
		// Set left & right offsets (same as in fs_read)
		leftOff = 0;
		if (i == 0)
			leftOff = filedes[fdInd].offset % BLOCK_SIZE;

		rightOff = 0;
		if (toWrite + leftOff < BLOCK_SIZE) {
			rightOff = BLOCK_SIZE - toWrite - leftOff;
		}
		
		// Read whole block into bounce buffer
		void *bBuf = malloc(BLOCK_SIZE);
		if (dataBlk_index(fd) == -1) { // new block must be allocated
			if (num_free_fat() <= 0) { // if disk is full
				set_file_size(fd, count - toWrite);
				return count - toWrite; // return the number of bytes sucessfully written
			} else { // otherwise, allocate a new block
				allocate_block(fd);	
			}				
		} else {
			block_read(dataBlk_index(fd), bBuf);
		}

		// write into bounce buffer, with the offsets in mind
		bWritten = BLOCK_SIZE - leftOff - rightOff;
		memcpy((char*)bBuf+leftOff, (char*)buf+bufOff, bWritten); // (dest, src, length in bytes)

		block_write(dataBlk_index(fd), bBuf); // write dirty block back to disk

		// Update status variables accordingly
		filedes[fd].offset = filedes[fd].offset + bWritten; // update file offset
		bufOff = bufOff + bWritten; // update read buffer offset
		toWrite = toWrite - bWritten; // update # of bytes to read
		free(bBuf); 
		i++;
	}

	set_file_size(fd, count - toWrite);
	return count - toWrite; // return the number of bytes sucessfully written
}


