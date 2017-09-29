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
#define NB_PAGE 8192
#define FILE_NAME 32
#define NB_LINK 3
#define NB_FILE 3
#define NB_FAT_NODE 42
#define NB_FREE_BLOCK_LIST 128
typedef struct FileAccessStatus {//12 byte

    int opened;
    int wr_ptr;
    int rd_ptr;

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

typedef struct fat_node {//12 byte

    int index;
    int next;
    int free;

} fat_node;

typedef struct FileAllocationTable {//512 byte

    fat_node table[NB_FAT_NODE];//128
    int      count;
    int      index;

} FileAllocationTable;

typedef struct freeblocklist {//512 byte

    int list[NB_FREE_BLOCK_LIST];

} freeblocklist;

#endif
