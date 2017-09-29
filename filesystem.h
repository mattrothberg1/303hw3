#ifndef FILESYSTEM_H
#define FILESYSTEM_H

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


#endif
