#ifndef STRUCTS_H
#define STRUCTS_H
#include<time.h>
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
#define FATNODE_SIZE 8
#define NB_BLOCK 8192
#define FILE_NAME 32
#define NB_LINK 3
#define NB_FILE 3
#define DISK_BLOCK_START 0
#define FREE_BLOCK_LIST_START DISK_BLOCK_START
#define NB_FREE_BLOCK_LIST 64
#define FILE_ALLOCATION_TABLE_START 64
#define NB_FILE_ALLOCATION_TABLE 128
#define DATA_BLOCK_START 192
#define ROOT DATA_BLOCK_START
#define NO_NEXT -2
#define FATHER_OF_ROOT -1
#define FREE 1
#define BUSY 0

typedef struct FileAccessStatus {//12 byte

    int open;
    int write;
    int read

} FileAccessStatus;
typedef struct Link {//32 byte
    char filename[FILE_NAME];//32
} Link;
typedef struct FileDescriptor {//160 byte

    char             filename[FILE_NAME];//32
    int              index;
    time_t           timestamp;
    int              size;
    FileAccessStatus fas;
    Link             links[NB_LINK];//3
} FileDescriptor;

typedef struct DirectoryDescriptor {//488 byte

    FileDescriptor table[NB_FILE];//3
    int            count;
    int            index;
} DirectoryDescriptor;

typedef struct FatNode {//8 byte

    int next;
    int free;

} fat_node;

typedef struct FileAllocationTable { //128 block

    fat_node table[NB_BLOCK];

} FileAllocationTable;

typedef struct FreeBlockList {//64 block

    int list[NB_BLOCK];

} FreeBlockList;
int write_blocks(int start, int size, void *buffer, void* map);
int write_FatNode(int index, FileAllocationTable* fat);
int read_blocks(int start, int size, void *buffer, void * map);
int init_disk(FILE * f);
int write_FreeListNode;
int init_fs(FileAllocationTable* fat, DirectoryDescriptor* root, FreeBlockList* fbl, void* map);
int verify_fs(&fat, &root, &fbl, map);

#endif
