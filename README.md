# Introduction
Authors: Hyoung-yoon Kim  
This project involved a simplified implementation of the FAT file system.   

# Data Structures

We had three data structures representing the superblock, FAT array and Root  
directory. We intialized global instances of these structs so that they would  
be accessible by all of our functions.   

We used the `__attribute__((__packed__))` to ensure that our metadata was  
exactly where we wanted it to be.

# Phase 1: Mounting/unmounting

The purpose of the function `fs_mount()` is to initialize our internal data  
structures based on the meta-data placed in the first few blocks of the disk. We  
complete this task in order. Firstly, the function `sb_init()` reads the first  
block of the disk to obtain the information in superblock. It also verifies that  
the overall format of the data is as expected. Initially, we implemented this  
phase by manually iterating through the block of data and setting our data  
structures. We later simplified the implementation significantly by taking  
advantage of C's structure packing. By molding the data structures according to  
the rules by which the data is stored in the disks, we reduced the whole  
procedure into a few block readings. Subsequently, `fat_init()` and  
`root_init()` read in FAT blocks and root directory block, respectively. The  
mount function also calls `fd_init()` to initialize the data structure for file  
descriptors. This operation is independent of the disk data.    

The function `fs_umount()` performs the reverse operation. That is, it writes  
back to the disk the meta-data that may have been modified in our internal data  
sturctures.    

Some minor functions include `num_free_fat()`, which chekcs the number of empty  
data blocks by checking if the entry of FAT is 0, or `fs_info()`, which displays  
the data obtained from the superblock. Also, note that the instances of the  
internal data structures are declared globally and statically.    


# Phase 2: File creation/deletion

In this phase we wrote `fs_create()` and `fs_delete()`. For both functions we  
used a helper function to check the validity of the filename, and did   
preliminary checks for the length of the filename and if the filename already   
existed in our root directory.   

In `fs_create()`, after doing the checks, we simply find an empty entry in the   
root directory, and initialize an empty file. We loop through the entries of the  
root directory and find an entry with the name as a null character. This   
involves setting the size of the file to 0, setting the name of the file to the   
given filename, and finding and initalizing an empty FAT entry in the FAT array. 
We wrote a helper function to help us find an empty entry in the FAT array,  
which looks for an entry in the FAT array with content 0.  

`fs_delete()` is simlar to `fs_create()`, however, instead of searching for an  
emtpy name in the root directory, we look for the given name. We then clear the  
name in the root directory by setting it to the null character, and then use   
another loop to clear the FAT entries for the file. We found it natural to use   
an iterator to do this because the FAT array is basicaly a linked list.   

# Phase 3: File descriptor operations

A file descriptor allows the user to conveniently read or make modifications in  
a file. We created the following simple structure to contain the information  
about a file's status:   

```
typedef struct FD{
	int id;
	int offset;
	int index; // index of the file of the root directory 
}FD;
```

The function `fd_init()` initializes a global array of this structure during the  
disk mount process. An `id` value of -1 signifies that the file descriptor is  
empty.  

The function `fs_open()` locates the specified file in the root directory and  
creates a new entry in the array of file descriptors. Once this process is done,  
the name of the file does not play a role until the file is closed. An opened  
file is instead identified based on its `id`. We keep a global variable  
`idCount`, which is incremented every time a new `id` is assigned, to ensure  
that all file descriptors have unique identifications. Another global variable  
`numFilesOpen` ensures that we do not exceed the maximum number of open files.  

The offset of the file is set to zero here, but it should be updated after  
reading or writing the file (next Phase). To provide a simple way for the user  
to update a file's offset, we wrote the function `fs_lseek()`, which locates the  
file descriptor based on its `id` and sets the offset.  

The function `fs_close()` re-initializes the file descriptor with the specified  
`id`.    


# Phase 4: File reading/writing

We created a few helper functions to complete this phase. The most essential of  
these functions was `dataBlk_index()`, which returns the index of the block  
corresponding to the file and its offset.  

Noting that disk reading or writing can only be done by one block at a time, we  
concluded that an implementation of this phase essentially amounts to computing  
and using the offsets to handle the "bounce buffer" properly.  

The "bounce buffer" is nothing but a block of data read from the disk. The  
variable `leftOff`, which represents the offset from the left, can be non-zero  
only in the first iteration of block read if the file's current offset does not  
align with the blocks. Therefore, a simple remainder operation should give us  
the value of `leftOff`. The variable `rightOff`, which represents the offset from   
the right, can be non-zero only at the last iteration of block read. Hence, its  
value is simply a subtraction of the number of bytes we have to read or write  
from the block size. Since the whole operation may be done in a single block, we  
also have to subtract `leftOff`.   

In case of reading, we simply copy from the bounce buffer to the read buffer in  
each iteration based on the computed offsets. For writing, the procedure  
requires a few more steps: First, we check if a new block must be allocated,  
depending on whether `dataBlk_index()` fails (returns -1). After this step, we  
obtain a block-sized buffer that contains the current disk data or is empty. We  
overwrite to a part of this buffer based on the computed offsets, and  
subsequently we place the buffer back in the block. The following code snippet  
shows how we computed the offsets:   

```
// Set left & right offsets (same as in fs_read)
leftOff = 0;
if (i == 0)
	leftOff = filedes[fdInd].offset % BLOCK_SIZE;

rightOff = 0;
if (toWrite + leftOff < BLOCK_SIZE) {
	rightOff = BLOCK_SIZE - toWrite - leftOff;
}
```    

# Testing 
To test our functions we wrote simple.c and test_read.c. simple.c tested phases 
1-3.  

test_read.c does a basic read from a hello.txt file. We manually added this file  
to the disk using the reference program. Then we manually tested phase 4 by with   
files of various sizes that we created. We tested writing and reading large   
blocks, setting the offset, and files that were larger than the disk capacity. 
We also compared it against the reference program and given tester. 

