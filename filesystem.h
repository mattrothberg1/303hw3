#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "structs.h"
#include <stdio.h>
/*
 *	Prototypes for our filesystem functions.
 *
 *
 */

//Help dialog
void help(char *progname);

//Main filesystem loop
void filesystem(char *file);

//Converts source data into appropriate binary data.
//User must free the returned pointer
char* generateData(char *source, size_t size);
int dump(FILE* fout,int pageNum);
int dumpBinary(char* fileName, int pageNum);
int usage();
int pwd();
int cd(char* file);
int ls();
int makedir(char * dName);
int cat(char* fileName);
int writef(char *fileName, int amt, char *data);
int append(char* fileName, int amt, char *data);
int removef(char * fileName,int start, int end);
int getpages(char * fileName);
int getf(char* fileName, int start, int end);
int rmdirc(char* fileName);
int rmForce(char * fileName);
int rmfile(char* fileName);
int scandisk();
int undelete(char * fileName);

#endif
