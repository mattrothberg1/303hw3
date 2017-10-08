#ifndef STRUCTS_H
#define STRUCTS_H
#include<time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
/*
 * .
 * Define page/sector structures here as well as utility structures
 * such as directory entries.
 *
 * Sectors/Pages are 512 bytes
 * The filesystem is 4 megabytes in size.
 * You will have 8K pages total.
 *
 */
#define BLOCK_SIZE 512
#define NB_BLOCK 8192
#define FILE_NAME 32
#define NB_LINK 3
#define NB_FILE 3
#define DISK_BLOCK_START 0
#define IS_DIRECTORY_START 0
#define IS_DIRECTORY_SIZE 64
#define FILE_ALLOCATION_TABLE_START 64
#define FILE_ALLOCATION_TABLE_SIZE 192
#define GARBAGE_START 256
#define GARBAGE_SIZE 11
#define DATA_BLOCK_START 267
#define ROOT DATA_BLOCK_START
#define NO_NEXT -2
#define FATHER_OF_ROOT -1
#define FREE 1
#define BUSY 0
#define ISDIRECTORY 1
#define NOTDIRECTORY 0
#define PATH_DEPTH 32
#define NB_GARBAGE 128

typedef struct FileAccessStatus {//12 byte

    int open;
    int write;
    int read

}FileAccessStatus;
typedef struct Link {//32 byte
    char filename[FILE_NAME];//32
}Link;
typedef struct FileDescriptor {//160 byte

    char             filename[FILE_NAME];//32
    int              index;
    time_t           timestamp;
    int              size;
    FileAccessStatus fas;
    Link             links[NB_LINK];//3

}FileDescriptor;

typedef struct DirectoryDescriptor {//488 byte

    FileDescriptor table[NB_FILE];//3
    int            count;
    int            index;

}DirectoryDescriptor;

typedef struct FatNode {//12 byte

    int next;
    int free;
    int signature;

}FatNode;
typedef struct Gabage {//44 byte
    char filename[FILE_NAME];
    int directoryIndex;
    int index;
    int signature;
}Garbage;
int write_blocks(int start, int size, void *buffer, void* map);
int write_FatNode(int index, FatNode* fat);
int read_blocks(int start, int size, void *buffer, void * map);
int init_disk(FILE * f);
int write_Garbage(int index, Garbage* garbage);
int write_IsDirectory(int index, int* isDirectory);
int init_fs(FatNode* fat, DirectoryDescriptor* root, int* isDirectory, Garbage* garbage, void* map);
int verify_fs(FatNode* fat, DirectoryDescriptor* root, int *id, void *map);
#endif
