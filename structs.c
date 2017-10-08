#include"structs.h"
#include<time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "support.h"
#include "structs.h"
#include "filesystem.h"
#include <sys/mman.h>
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


int init_disk(FILE * f){
	char* clean = malloc(BLOCK_SIZE * NB_BLOCK);
	fwrite(clean,BLOCK_SIZE, NB_BLOCK,f);
	fflush(f);
	//all done;
	return 1;
}

int write_FatNode(int index, FatNode* fat) {
	msync(fat + index * sizeof(FatNode), sizeof(FatNode), MS_SYNC);
	return 1;
}

int write_IsDirectory(int index, int* isDirectory) {
	msync(isDirectory + index * sizeof(int), sizeof(int), MS_SYNC);
	return 1;
}
int write_Garbage(int index, Garbage* garbage) {
    //printf("write %i to garbage",index);
    msync(garbage + index * sizeof(Garbage),sizeof(Garbage), MS_SYNC);
    return 1;
}

int init_fs(FatNode* fat, DirectoryDescriptor* root, int* isDirectory, Garbage* garbage, void* map) {
	//1. intialize idl
	isDirectory = malloc(sizeof(int) * NB_BLOCK);
	//before the block which the record of ROOT , we set all records NOTDIRECTORY
	for(int i = 0; i < ROOT;i++) {
		isDirectory[i] = NOTDIRECTORY;
	}
	//only the ROOT now ISDIRECTORY
	isDirectory[ROOT] = ISDIRECTORY;

	//after the block which the record of ROOT locate, we set all records NOTDIRECTORY
	for(int i = ROOT + 1; i < NB_BLOCK; i++) {
	isDirectory[i] = NOTDIRECTORY;
}
	//write to disk
	memcpy(map, isDirectory, IS_DIRECTORY_SIZE * BLOCK_SIZE);
	msync(map, IS_DIRECTORY_SIZE * BLOCK_SIZE, MS_SYNC);
	free(isDirectory);
	//2. inicialize fat

	fat = (FatNode*)malloc(sizeof(FatNode) * NB_BLOCK);
	FatNode fn;
	//the fat_nodes before ROOT are all BUSY and have NO_NEXT so as the ROOT
	fn.next = NO_NEXT;
	fn.signature = 0;
	fn.free = BUSY;
	for (int i = 0; i <=ROOT; i++){
		fat[i] = fn;
	}

	//the fat_nodes after ROOT are all FREE and have NO_NEXT
	fn.free = FREE;
	for (int i = ROOT + 1; i < NB_BLOCK; i++) {
		fat[i] = fn;
	}
    //set signature = 0;

	//write to disk
	memcpy(map + FILE_ALLOCATION_TABLE_START * BLOCK_SIZE, fat, FILE_ALLOCATION_TABLE_SIZE * BLOCK_SIZE);
	msync(map + FILE_ALLOCATION_TABLE_START * BLOCK_SIZE, FILE_ALLOCATION_TABLE_SIZE * BLOCK_SIZE, MS_SYNC);
	free(fat);


	//3.initialize garbage
	garbage = malloc(sizeof(Garbage) * NB_GARBAGE);
	Garbage g;
	char* gname = "";
	strcpy(g.filename,gname);
	g.index = 0;
	g.directoryIndex = 0;
	g.signature = 0;
	for(int i = 0; i < NB_GARBAGE;i++) {
        garbage[i] = g;
	}
	memcpy(map + GARBAGE_START * BLOCK_SIZE, garbage,GARBAGE_SIZE * BLOCK_SIZE);
	msync(map + GARBAGE_START * BLOCK_SIZE, GARBAGE_SIZE * BLOCK_SIZE,MS_SYNC);
	free(garbage);
	//4. initialize root

	root = (DirectoryDescriptor*)malloc(sizeof(DirectoryDescriptor));
	//self
	memset(root, 0, sizeof(DirectoryDescriptor));
	FileDescriptor fd1;
	memset(&fd1,0,sizeof(FileDescriptor));
	char* fname = ".";
	strcpy(fd1.filename, fname);
	fd1.index = ROOT;
	fd1.timestamp = time(NULL);

	//father
	FileDescriptor fd2;
	memset(&fd2,0,sizeof(FileDescriptor));
	fname = "..";
	strcpy(fd2.filename, fname);
	fd2.index = -1;

	(*root).table[0] = fd1;
	(*root).table[1] = fd2;
	(*root).index = ROOT;
	(*root).count = 2;

	//write to disk
	write_blocks(ROOT,1,(void*)root,map);
    free(root);
	//all done
	return 1;
}

int verify_fs(FatNode* fat, DirectoryDescriptor* root, int* id, void* map) {
	if (*(char*)fat != 0 && *(char*)root != 0) {
		return 1;
	}
	return 0;
}

