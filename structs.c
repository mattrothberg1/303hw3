#include"structs.h"
#include<time.h>
/*
 *
 * Definition of structs in structs.h
 *
 */

int write_blocks(int start, int size, void *buffer, void* map) {
    memcpy(map + start * BLOCK_SIZE, buffer, size * BLOCK_SIZE);
	msync(map + start * BLOCK_SIZE, size * BLOCK_SIZE, MS_SYNC);
	//all done
	return 1;
}

int read_blocks(int start, int size, void *buffer, void * map) {
	memcpy(buffer, map + start * BLOCK_SIZE, size * BLOCK_SIZE);
	//all done
	return 1;
}

int init_disk(FILE * f){
    size = BLOCK_SIZE * NB_BLOCK;
	char clean[size] = "";
	fwrite(clean,1,size,f);	
	fflush(f);
	//all done;
	return 1;
}

int write_FatNode(int index, FileAllocationTable* fat) {
	msync(fat + index * FATNODE_SIZE + index, FATNODE_SIZE, MS_SYNC);
}

int write_FreeListNode(int index, FreeBlockList* fbl) {
	
	msync(fbl + index * sizeof(int), sizeof(int), MS_SYNC);
}

int init_fs(FileAllocationTable* fat, DirectoryDescriptor* root, FreeBlockList* fbl, void* map) {
	//1. intialize fbl
	
	//before the block which the record of ROOT locate also the ROOT, we set all records BUSY
	for(int i = 0; i <= ROOT;i++) {
		fbl.list[i] = BUSY;
	}

	//after the block which the record of ROOT locate, we set all records FREE
	for(int i = ROOT + 1; i < NB_BLOCK; i++) {
	fbl.list[i] = FREE;
}
	//write to disk
	memcpy(map + DISK_BLOCK_START * BLOCK_SIZE, fbl, NB_FREE_BLOCK_LIST * BLOCK_SIZE);
	msync(map + DISK_BLOCK_START * BLOCK_SIZE, NB_FREE_BLOCK_LIST * BLOCK_SIZE, MS_SYNC);

	//2. inicialize fat

	FatNode fn;	
	//the fat_nodes before ROOT are all BUSY and have NO_NEXT so as the ROOT
	fn.next = NO_NEXT;
	fn.free = BUSY;	
	for (int i = 0; i <=ROOT; i++){	
		fat.table[i] = fn;
	}

	//the fat_nodes after ROOT are all FREE and have NO_NEXT 
	fn.free = FREE;
	for (int i = ROOT + 1; i < NB_BLOCK; i++) {
		fat.table[i] = fn;
	}

	//write to disk
	memcpy(map + FILE_ALLOCATION_TABLE_START * BLOCK_SIZE, fat, NB_FILE_ALLOCATION_TABLE * BLOCK_SIZE);
	msync(map + FILE_ALLOCATION_TABLE_START * BLOCK_SIZE, NB_FILE_ALLOCATION_TABLE * BLOCK_SIZE, MS_SYNC);	

	//3. initialize root
	
	//self
	FileDescriptor fd1;
	fd1.filename = '.';
	fd1.index = ROOT;
	fd1.timestamp = time(NULL);
	fd1.size = FATHER_OF_ROOT;
	fd1.fas = NULL;
	fd1.links = NULL;
	
	//father
	FileDescriptor fd2;
	fd1.filename = "..";
	fd1.index = -1;
	fd1.timestamp = NULL
	fd1.size = NULL
	fd1.fas = NULL;
	fd1.links = NULL;
	
	root.table[0] = fd1;
	root.table[1] = fd2;
	root.index = ROOT;
	root.count = 2;
	
	//write to disk
	write_blocks(DATA_BLOCK_START,1,root,map);

	//all done
	return 1;	
}

int verify_fs(FileAllocationTable* fat, DirectoryDescriptor* root, FreeBlockList* fbl, void* map) {
	fbl = (FreeBlockList*)(map + DISK_BLOCK_START * BLOCK_SIZE);
	fat = (FileAllocationTable*)(map + FILE_ALLOCATION_TABLE_START * BLOCK_SIZE);
	root = (DirectoryDescriptor*)(map + DATA_BLOCK_START * BLOCK_SIZE);
	if ((*fat) != NULL && (*root) != NULL) {
		return 1;
	}
	return 0;
}

