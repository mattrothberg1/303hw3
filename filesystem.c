#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "support.h"
#include "structs.h"
#include <malloc.h>
#include <limits.h>
#include <time.h>

DirectoryDescriptor *root = NULL;
DirectoryDescriptor *directory_cl = NULL;//present location
FatNode *fat = NULL;
Garbage * garbage = NULL;
int *id = NULL;
FILE * f = NULL;
int fs;
void *map = NULL;

/*
 * generateData() - Converts source from hex digits to
 * binary data. Returns allocated pointer to data
 * of size amt/2.
 */

char* generateData(char *source, size_t size)
{
	char *retval = (char *)malloc((size >> 1) * sizeof(char));

	for(size_t i=0; i<(size-1); i+=2)
	{
		sscanf(&source[i], "%2hhx", &retval[i>>1]);
	}
	return retval;

}
int setIsDirectory(int index,int status) {
    id[index] = status;
    write_IsDirectory(index,id);
    //printf("%s","new directory\n");
    return 1;
}
int isFileExistD(char * filename, DirectoryDescriptor *dd) {
    while (fat[dd->index].next!=NO_NEXT && fat[dd->index].free == BUSY) {
        for(int i = 0; i < dd->count; i++) {
            if(!strcmp(filename,dd->table[i].filename)) {
                return dd->table[i].index;
            }
        }
        dd = (DirectoryDescriptor*)(map + fat[dd->index].next*BLOCK_SIZE);
    }
    for(int i = 0; i < dd->count; i++) {
        if(!strcmp(filename,dd->table[i].filename)) {
            return dd->table[i].index;
        }
    }
    return 0;
}
int findFreeBlock(){
	int blockStart;
	for (blockStart = ROOT + 1; blockStart < NB_BLOCK; blockStart++) {
		if (fat[blockStart].free == FREE) {
			break;
		}
	}
	if (blockStart == NB_BLOCK) {
		return 0;
	}
	return blockStart;
}
int setFreeBlock(int index, int status) {
    fat[index].free = status;
    write_FatNode(index,fat);
    return 1;
}
int setFatNext(int index,int next) {
    fat[index].next = next;
    write_FatNode(index,fat);
    return 1;
}
int setSignature(int index, int signature) {
    fat[index].signature = signature;
    write_FatNode(index,fat);
    return 1;
}
int createDirectoryDiscriptor(DirectoryDescriptor * dd) {
	int blockStart = findFreeBlock();
	if (!blockStart) {
		return 0;
	}
	//set newDD
	DirectoryDescriptor newDD;
	memset(&newDD,0,sizeof(newDD));
	newDD.index = blockStart;
	newDD.count = 0;
	//write newDD to disk
	write_blocks(blockStart,1,(void*)&newDD,map);
	//modify fat[newDD.index]
	setFreeBlock(blockStart,BUSY);
	setFatNext(blockStart,NO_NEXT);
	setSignature(blockStart,0);
	//modify isDirectory
	setIsDirectory(newDD.index,ISDIRECTORY);

	//modify fat[dd->index]
	//printf("%i",dd->index);
	setFatNext(dd->index,blockStart);


	return blockStart;
}
int createFile(char * filename) {//crate file in current directory

    if(isFileExistD(filename,directory_cl)) {
        printf("ERROR: file exist\n");
        return 0;
	};
	int blockStart = findFreeBlock();
	if(!blockStart){
        printf("ERROR: NO SPACE\n");
		return 0;
	}
	//printf("%i",blockStart);
	DirectoryDescriptor * tempD = directory_cl;
    //DirectoryDescriptor * parent;
	while(tempD->count == 3 ) {
		int next = fat[tempD->index].next;
		if (next == NO_NEXT) {
            setFreeBlock(blockStart,BUSY);
            int directoryStart = createDirectoryDiscriptor(tempD);
			if(!directoryStart) {
				return 0;
			} else {
			    tempD = (DirectoryDescriptor*)(map + directoryStart * BLOCK_SIZE);
			    //printf("\n%s %i\n","i get tempd",directoryStart);
				break;
			}
			setFreeBlock(blockStart,FREE);
		}
		//parent = tempD;
		tempD = (DirectoryDescriptor*)(map + next*BLOCK_SIZE);
	}


	for(int i = 0; i < 3; i++) {
		if ((*(char*)&(tempD->table[i])) == 0) {
            //reset datablock
			void * buffer = malloc(BLOCK_SIZE);
			memset(buffer,0,BLOCK_SIZE);
			write_blocks(blockStart, 1, buffer, map);
			free(buffer);
			//set fat
			setFreeBlock(blockStart,BUSY);
			setFatNext(blockStart,NO_NEXT);
			setSignature(blockStart,0);
			//set id
			setIsDirectory(blockStart,NOTDIRECTORY);

            //set directoryDiscriptor

			FileDescriptor fd;
			memset(&fd,0,sizeof(fd));
			strcpy(fd.filename,filename);
			fd.index = blockStart;
			fd.timestamp = time(NULL);
			fd.size = 1;

			tempD->table[i] = fd;
			tempD->count+=1;
			//write tempD to disk
			msync(map + (tempD->index) * BLOCK_SIZE, BLOCK_SIZE, MS_SYNC);




			return blockStart;
		}
	}
	return 0;
}
int dump(FILE* fout,int pageNum) {
    char* buffer = (char*)(map + pageNum * BLOCK_SIZE);
    char * out = malloc(BLOCK_SIZE);
    memcpy(out,buffer,BLOCK_SIZE);
	/*TODO
	use fprintf to printout in hex format
	*/
	for (int i = 0; i < BLOCK_SIZE; i++) {
        fprintf(fout,"%02hhx ",out[i]);

	}
	//all done;
	printf("\n");
	free(out);
	return 1;
}
int dumpBinary(char* fileName, int pageNum) {
	if (pageNum >= NB_BLOCK) {
		printf("ERROR:out of disk\n");
		return 0;
	}
	int file = isFileExistD(fileName,directory_cl);
	if(!file) {
        file = createFile(fileName);
	}
	memcpy(map + file * BLOCK_SIZE, map + pageNum * BLOCK_SIZE, BLOCK_SIZE);
	msync(map + file * BLOCK_SIZE, BLOCK_SIZE, MS_SYNC);
	setFreeBlock(file,BUSY);
	return file;

	return 1;
}
int usage() {
    int fs = 0;
    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (fat[i].free == BUSY) {
            fs+=1;
        }
    }
    int f = fs;
    for (int i = 0; i < BLOCK_SIZE; i++) {
        if(id[i] == ISDIRECTORY)
        f--;
    }
    f-=ROOT;
    int fsb = fs * 512;
    int fb = f * 512;
    printf("filesystem use %i pages, %i bytes\nfile use %i pages, %i bytes\n",fs,fsb,f,fb);
    return 1;
}

int makedir(char * dName) {
    if(isFileExistD(dName,directory_cl) && id[isFileExistD(dName,directory_cl)] == ISDIRECTORY) {
        printf("ERROR: Directory already exist\n");
        return 0;
    }
    if(isFileExistD(dName,directory_cl) && id[isFileExistD(dName,directory_cl)] != ISDIRECTORY) {
        printf("ERROR: file name conflict, change a name\n");
        return 0;
    }
    int blockStart = findFreeBlock();
	if (!blockStart) {
		return 0;
	}
	time_t now = time(NULL);
	//set self and father

	//set self
    FileDescriptor fd1;
	memset(&fd1,0,sizeof(FileDescriptor));
	char* fname1 = ".";
	strcpy(fd1.filename, fname1);
	fd1.index = blockStart;
	fd1.timestamp = now;
	//set father
    FileDescriptor fd2;
	memset(&fd2,0,sizeof(FileDescriptor));
	char* fname2 = "..";
	strcpy(fd2.filename, fname2);
	fd2.index = directory_cl->index;
	fd2.timestamp = now;

	//set newDD
	DirectoryDescriptor newDD;
	memset(&newDD,0,sizeof(newDD));
	newDD.index = blockStart;
	newDD.count = 2;
	newDD.table[0] = fd1;
	newDD.table[1] = fd2;

	//write newDD to disk
	write_blocks(blockStart,1,(void*)&newDD,map);

	//modify isDirectory
	setIsDirectory(newDD.index,ISDIRECTORY);

	//put newDD to directory_cl
    DirectoryDescriptor * tempD = directory_cl;

	while(tempD->count == 3) {
		int next = fat[tempD->index].next;
		if (next == NO_NEXT) {
            setFreeBlock(blockStart,BUSY);
            int directoryStart = createDirectoryDiscriptor(tempD);
			if(!directoryStart) {
				return 0;
			} else {
			    tempD = (DirectoryDescriptor*)(map + directoryStart * BLOCK_SIZE);
			    //printf("\n%s %i\n","i get tempd",directoryStart);
				break;
			}
			setFreeBlock(blockStart,FREE);
		}
		tempD = (DirectoryDescriptor*)(map + next*BLOCK_SIZE);
	}
    for(int i = 0; i < 3; i++) {
        if (!(*(char*)&(tempD->table[i]))) {
                strcpy(tempD->table[i].filename,dName);
                tempD->table[i].index = blockStart;
                tempD->table[i].timestamp = now;
                tempD->count++;
                msync(map + (tempD->index) * BLOCK_SIZE, BLOCK_SIZE, MS_SYNC);
                break;
        }

    }
    //modify fat[newDD.index] (this should be do at last)
	setFreeBlock(blockStart, BUSY);
    setFatNext(blockStart, NO_NEXT);
    setSignature(blockStart, 0);
	return blockStart;
}
int getFilesD(DirectoryDescriptor* dd, int * files) {// return fileNum

    int j = 0;
    DirectoryDescriptor* tempD = dd;
    for(int i = 0; i < 3; i++) {
        if (*(char*)&(tempD->table[i])) {
            files[j++] = tempD->table[i].index;
        }
    }
   while(fat[tempD->index].next!=NO_NEXT) {
        tempD = (DirectoryDescriptor*)(map + fat[tempD->index].next * BLOCK_SIZE);
        if(fat[tempD->index].free == FREE) {// actually this won't happen because when we write anything to disk, we will change it's next to NO_NEXT
            break;
        }
        for(int i = 0; i < 3; i++) {
            if (*(char*)&(tempD->table[i])) {
                files[j++] = tempD->table[i].index;
            }
        }
    }
    return j;
}
int getFileSize(int file) {
    int size = 0;
    /*if(id[file]==ISDIRECTORY) {
        return size;
    }*/
    if(fat[file].free == BUSY){
        size++;
    }
    int tempf = file;
    while(fat[tempf].next != NO_NEXT) {
        tempf = fat[tempf].next;
        if(fat[tempf].free == BUSY) {
            size++;
        } else {
            break;
        }
    }
    return size;
}
void getFileSizeR(int *total,int file) {
    if(id[file] == NOTDIRECTORY) {
        (*total) += getFileSize(file);
        return;
    }
    if(id[file] == -1) {
        return;
    }
    DirectoryDescriptor * dd = (DirectoryDescriptor *)(map + file * BLOCK_SIZE);
    int *files = malloc(sizeof(int) * NB_BLOCK);
    memset(files,0,sizeof(int) * NB_BLOCK);
    for(int i = 2; i < getFilesD(dd,files); i++) {
        getFileSizeR(total,files[i]);
    }
    free(files);
    return;
}

int ls() {
    DirectoryDescriptor* dd = directory_cl;

    for(int i = 0; i < 3; i++) {
        if (*(char*)&(dd->table[i])) {
            char name[FILE_NAME] = "";
            strcpy(name,dd->table[i].filename);
            int size = 0;
            //printf("%i",id[dd->table[i].index]);
            if(dd->table[i].index == -1) {
                printf("d      unknown %s\n",name);
            }
            else {
                char isd;
                if(id[dd->table[i].index] == ISDIRECTORY){
                    getFileSizeR(&size,dd->table[i].index);
                    isd = 'd';
                //printf("yes");
                } else {
                    size = getFileSize(dd->table[i].index);
                    isd = 'f';
                //printf("no");
                }
            printf("%c %12i %s\n",isd,size*BLOCK_SIZE,name);
            }
        }
    }
    while(fat[dd->index].next!=NO_NEXT) {
        dd = (DirectoryDescriptor*)(map + fat[dd->index].next * BLOCK_SIZE);
        for(int i = 0; i < 3; i++) {
            if (*(char*)&(dd->table[i])) {
                char name[FILE_NAME] = "";
                strcpy(name,dd->table[i].filename);
                int size = 0;
            if(dd->table[i].index == -1) {
                printf("d      unknown %s\n",name);
            } else {
                char isd;
                if(id[dd->table[i].index] == ISDIRECTORY){
                    getFileSizeR(&size,dd->table[i].index);
                    isd = 'd';
                //printf("yes");
                } else {
                    size = getFileSize(dd->table[i].index);
                    isd = 'f';
                //printf("no");
                }
                printf("%c %12i %s\n",isd,size*BLOCK_SIZE,name);
                }
            }
        }
    }
    return 1;
}
int cd(char* file) {
    int index = isFileExistD(file,directory_cl);
    if(!index) {
        printf("ERROR:directory not exist\n");
        return 0;
    }
    if (id[index] == NOTDIRECTORY) {
        printf("ERROR:directory not exist\n");
        return 0;
    }
    directory_cl = (DirectoryDescriptor*)(map + index * BLOCK_SIZE);
    return index;

}
char *getFileName(DirectoryDescriptor*dd, int fileIndex) {
    while (fat[dd->index].next!=NO_NEXT) {
        for(int i = 0; i < dd->count; i++) {
            if(fileIndex == dd->table[i].index) {
                return dd->table[i].filename;
            }
        }
        dd = (DirectoryDescriptor*)(map + fat[dd->index].next*BLOCK_SIZE);
    }
    for(int i = 0; i < dd->count; i++) {
        if(fileIndex == dd->table[i].index) {
                return dd->table[i].filename;
        }
    }
    return "";
}
int pwd() {
    DirectoryDescriptor * tempD = directory_cl;
    char * path[PATH_DEPTH];
    int i = 0;
    while(tempD->table[0].index!=ROOT) {
        int index = tempD->table[0].index;
        tempD = (DirectoryDescriptor*)(map + tempD->table[1].index * BLOCK_SIZE);
        path[i] = malloc(FILE_NAME);
        memset(path[i],0,FILE_NAME);
        char *filename = getFileName(tempD,index);
        strcpy(path[i],filename);
        i++;
    }
    i--;
    printf("/home");
    for(;i>=0;i--) {
        printf("/%s",path[i]);
        free(path[i]);
    }
    printf("\n");
    return 1;

}
int cat(char* fileName) {
    int file = isFileExistD(fileName,directory_cl);

    if(!file) {
        printf("ERROR: file not exist\n");
        return 0;
    }
    if(id[file]==ISDIRECTORY) {
        printf("ERROR: it's a directory\n");
        return 0;
    }
    char * buffer = (char*)malloc(BLOCK_SIZE);
    memcpy(buffer,map + file * BLOCK_SIZE,BLOCK_SIZE);
    for(int i = 0; i < BLOCK_SIZE;i++) {
        printf("%c",buffer[i]);
    }

    while (fat[file].next!=NO_NEXT) {
        file = fat[file].next;
        memcpy(buffer,map + file*BLOCK_SIZE,BLOCK_SIZE);
        for(int i = 0; i < BLOCK_SIZE;i++) {
            printf("%c",buffer[i]);
        }
    }
    printf("\n");
    free(buffer);
    return file;
}
DirectoryDescriptor* getDD(int file, int * rank) {
    //printf("enter with file %i\n",file);

    DirectoryDescriptor * tempD = directory_cl;

    while(fat[tempD->index].next!=NO_NEXT) {
        for(int i = 0; i < 3; i++) {
            if (tempD->table[i].index == file) {
                *rank = i;
                //printf("DD is %i", tempD->table[i].index);
                return tempD;
            }
        }
        tempD = map + fat[tempD->index].next * BLOCK_SIZE;
    }

    for(int i = 0; i < 3; i++) {
        if (tempD->table[i].index == file) {
            //printf("DD is %i", tempD->table[i].index);

            (*rank) = i;
            return tempD;
        }
    }
    //may need to be change
    return tempD;
}
int writef(char *fileName, int amt, char *data) {
    int file = isFileExistD(fileName,directory_cl);
    //printf("file is %i",99);
    if(file && id[file]==ISDIRECTORY) {
        printf("ERROR: it's a directory\n");
        return 0;
    }
    if(!file) {
        file = createFile(fileName);
    }
    //printf("i'm the first original block %i\n",file);
    int blockO = getFileSize(file);
    //printf("original file size %i\n",blockO);

    if(blockO * BLOCK_SIZE < amt) {
        int moreBlock = (amt % BLOCK_SIZE == 0 ? (amt - blockO * BLOCK_SIZE)/BLOCK_SIZE : (amt - blockO * BLOCK_SIZE)/BLOCK_SIZE + 1);
        //printf("moreBlock: %i",moreBlock);

        //printf("moreblock %i\n",moreBlock);
        int * block = malloc(moreBlock * sizeof(int));
        for(int i = 0; i < moreBlock; i++) {
            block[i] = findFreeBlock();
            setFreeBlock(block[i], BUSY);
        }
        for(int i = 0; i < moreBlock; i++) {
            setFreeBlock(block[i],FREE);
           // printf("assign block %i\n",block[i]);
        }
        void * buffer =  malloc(BLOCK_SIZE);
        int i = 0;
        for(; i < blockO; i++) {
            memcpy(buffer, data + i * BLOCK_SIZE, BLOCK_SIZE);
            write_blocks(file, 1, buffer, map);
            file = fat[file].next;
        }
        //printf("if %i == %i, pass\n",file, NO_NEXT);
        file = isFileExistD(fileName,directory_cl);
        for(int j = 0; j < blockO - 1; j++){
            file = fat[file].next;
        }
        //printf("i'm the last original block %i\n", file);
        //printf("if %i == %i, pass\n",i, blockO);
        for(; i < moreBlock + blockO - 1; i++) {
            memcpy(buffer,data + i * BLOCK_SIZE, BLOCK_SIZE);
            //printf("assigned block %i written\n",block[i - blockO]);
            write_blocks(block[i - blockO],1,buffer,map);
            //set fat
            setFatNext(file,block[i - blockO]);
            setFreeBlock(block[i - blockO],BUSY);
            setSignature(block[i-blockO],0);
            file = block[i - blockO];
            setIsDirectory(file,NOTDIRECTORY);
        }
        //printf("if %i the second last assigned block, pass\n",file);
        memset(buffer,0,BLOCK_SIZE);
        //printf("remain %i to be written\n",amt - (i * BLOCK_SIZE));
        //printf("written to assigned last block %i\n",block[i - blockO]);
        memcpy(buffer, data + i * BLOCK_SIZE, amt - (i * BLOCK_SIZE));
        //printf("prepare write to disk\n");

        write_blocks(block[i - blockO],1,buffer,map);
        //printf("finish written blocks\n");

        //set fat

        setFatNext(file,block[i - blockO]);
        setFreeBlock(block[i - blockO],BUSY);
        setFatNext(block[i - blockO],NO_NEXT);
        setSignature(block[i - blockO], 0);
        //set dd
        setIsDirectory(block[i - blockO], NOTDIRECTORY);

        //set fd in dd
        file = isFileExistD(fileName,directory_cl);
        DirectoryDescriptor* dd = getDD(file, &i);
        //printf("in DirectoryDescriptor %i",dd);
        //printf("index %i",i);


        dd->table[i].size = moreBlock + blockO;
        dd->table[i].timestamp = time(NULL);
        msync(dd,BLOCK_SIZE,MS_SYNC);
        free(buffer);
        free(block);

    } else {
        void * buffer = malloc(BLOCK_SIZE);
        memset(buffer,0,BLOCK_SIZE);
        int blockN = (amt % BLOCK_SIZE == 0 ? amt / BLOCK_SIZE : amt / BLOCK_SIZE + 1);
        int i = 0;
        for(; i < blockN - 1; i++) {
            memcpy(buffer, data + i * BLOCK_SIZE, BLOCK_SIZE);
            write_blocks(file, 1, buffer, map);
            file = fat[file].next;

        }
        memset(buffer,0,BLOCK_SIZE);
        memcpy(buffer, data + i * BLOCK_SIZE, amt - (i * BLOCK_SIZE));
        write_blocks(file,1,buffer,map);

        //set fat
        for(i = 0; i < blockN - 1; i++) {
            file = fat[file].next;
        }
        while(fat[file].next!=NO_NEXT){
            int tempNext = fat[file].next;
            setFatNext(file,NO_NEXT);
            file = tempNext;
            setFreeBlock(file,FREE);
        }
        //set dd
        int file = isFileExistD(fileName,directory_cl);
        DirectoryDescriptor* dd = getDD(file, &i);
        dd->table[i].size = blockN;
        dd->table[i].timestamp = time(NULL);
        msync(dd,BLOCK_SIZE,MS_SYNC);
        free(buffer);
    }
    return 1;
}

int append(char* fileName, int amt, char *data) {
    //printf("data is %s",data);
    int file = isFileExistD(fileName,directory_cl);
    if (!file) {
        printf("ERROR: file not exist\n");
        return 0;
    }
    if(id[file] == ISDIRECTORY) {
        printf("ERROR: it's a directory\n");
        return 0;
    }
    //printf("file at block %i\n",file);
    while(fat[file].next!=NO_NEXT) {
        file = fat[file].next;
    }
    //printf("append start from %i\n",file);

    char * buffer = malloc(BLOCK_SIZE);
    memcpy(buffer,map + file * BLOCK_SIZE,BLOCK_SIZE);
    //printf("file content:\n%s\n",buffer);
    int sizeU = strlen(buffer);
    //printf("file already in use %i bytes\n",sizeU);

    if(amt <= BLOCK_SIZE - sizeU) {
        char* newStr = malloc(BLOCK_SIZE);
        memset(newStr,0,BLOCK_SIZE);
        newStr = strcat(buffer,data);
        //printf("newstr:\n%s\n",newStr);
        write_blocks(file,1,newStr,map);
        free(newStr);
        //free(buffer);
        return 1;
    }
    //printf("need more blocks\n");
    if(BLOCK_SIZE != sizeU){
        //printf("sizeU: %i",sizeU);
        char* newStr = malloc(BLOCK_SIZE);
        char* str2 = malloc(BLOCK_SIZE - sizeU);
        memcpy(str2,data,BLOCK_SIZE - sizeU);
        newStr = strcat(buffer,str2);
        //printf("content of file's first block\n%s\n",newStr);
        write_blocks(file,1,newStr,map);
        free(newStr);
        free(str2);
    }

    buffer = malloc(BLOCK_SIZE);
    data = data + (BLOCK_SIZE - sizeU);
    amt = amt - (BLOCK_SIZE - sizeU);
    //printf("amt %i",amt);

    int blockN = (amt%BLOCK_SIZE == 0 ? amt/BLOCK_SIZE : amt/BLOCK_SIZE + 1);

    int * block = malloc(sizeof(int) * blockN);

    for(int i = 0; i < blockN; i++) {
        block[i] = findFreeBlock();
        setFreeBlock(block[i], BUSY);
    }
    for(int i = 0; i < blockN; i++) {
        setFreeBlock(block[i],FREE);
    }
    //printf("blockN: %i ",blockN);
    int i = 0;
    for(; i < blockN - 1; i++) {
        memcpy(buffer, data + i * BLOCK_SIZE,BLOCK_SIZE);
        write_blocks(block[i],1,buffer,map);
        setFreeBlock(block[i],BUSY);
        setFatNext(file,block[i]);
        setSignature(block[i],0);
        file = block[i];
        setIsDirectory(block[i],NOTDIRECTORY);
    }

    memset(buffer,0,BLOCK_SIZE);
    memcpy(buffer,data + i * BLOCK_SIZE, amt - i * BLOCK_SIZE);
    write_blocks(block[i], 1, buffer, map);
    setFreeBlock(block[i], BUSY);
    setFatNext(file, block[i]);
    setFatNext(block[i],NO_NEXT);
    setSignature(block[i], 0);
    setIsDirectory(block[i],NOTDIRECTORY);



    //set dd
    file = isFileExistD(fileName,directory_cl);
    DirectoryDescriptor* dd = getDD(file, &i);

    dd->table[i].size += blockN;
    dd->table[i].timestamp = time(NULL);
    msync(dd,BLOCK_SIZE,MS_SYNC);
    free(buffer);
    free(block);

    return 1;
}
int removef(char * fileName,int start, int end) {
    int file = isFileExistD(fileName,directory_cl);
    if (!file) {
       printf("ERROR: file not exist\n");
       return 0;
    }
    if(id[file] == ISDIRECTORY) {
        printf("ERROR: it's a directory\n");
    }
    int blockO = getFileSize(file);
    char *buffer = malloc(blockO * BLOCK_SIZE);
    for(int i = 0; i < blockO; i++) {
        memcpy(buffer + i * BLOCK_SIZE,map + file * BLOCK_SIZE,BLOCK_SIZE);
        file = fat[file].next;
    }

    char *str1 = malloc(start);
    memcpy(str1,buffer,start);
    char* str2 = malloc(strlen(buffer) - end);
    memcpy(str2,buffer+end,strlen(buffer) - end);
    char* newstr = malloc(strlen(buffer) - (end-start));
    newstr = strcat(str1,str2);
    writef(fileName,strlen(newstr),newstr);
    //sleep(0.1);
    //free(buffer);
    //free(newstr);
    //free(str2);
    return 1;
}
int getpages(char * fileName){
    int file = isFileExistD(fileName,directory_cl);
    if(!file) {
        perror("ERROR: file not exist");
    }
    if(id[file] == ISDIRECTORY) {
        DirectoryDescriptor* dd = map + file * BLOCK_SIZE;
        for(int i = 0; i < 3; i++) {
            if (*(char*)&(dd->table[i])) {
                printf("%i,",dd->table[i].index);
            }
        }
        while(fat[dd->index].next!=NO_NEXT) {
            dd = (DirectoryDescriptor*)(map + fat[dd->index].next * BLOCK_SIZE);
            for(int i = 0; i < 3; i++) {
                if (*(char*)&(dd->table[i])) {
                    printf("%i,",dd->table[i].index);
                }
            }
        }
    }else {
        while(fat[file].next!=NO_NEXT) {
            printf("%i,",file);
            file = fat[file].next;
        }
        printf("%i",file);
    }
    printf("\n");
    return 1;
}
int getf(char* fileName, int start, int end) {
    int file = isFileExistD(fileName,directory_cl);
    if (!file) {
       printf("ERROR: file not exist\n");
       return 0;
    }
    int blockO = getFileSize(file);
    //printf("%i",blockO);
    char *buffer = malloc(blockO * BLOCK_SIZE);
    for(int i = 0; i < blockO; i++) {
        memcpy(buffer + i * BLOCK_SIZE,map + file * BLOCK_SIZE,BLOCK_SIZE);
        file = fat[file].next;
    }
    char* out = malloc(end - start);
    memcpy(out, buffer+start, end - start);
    for(int i = 0; i < end - start; i++) {
        printf("%c",out[i]);
    }
    printf("\n");
    free(buffer);
    free(out);
    return 1;
}
int rmdirc(char* fileName){
    int file = isFileExistD(fileName,directory_cl);
    if(!file) {
        perror("ERROR: file not exist\n");
        return 0;
    }
    if(id[file] == NOTDIRECTORY) {
        perror("ERROR: it's not directory");
        return 0;
    }
    DirectoryDescriptor* dd = map + file * BLOCK_SIZE;
    int* files = malloc(NB_BLOCK * sizeof(int));
    int fileNum = getFilesD(dd,files);
    if(fileNum > 2) {
        perror("ERROR: can't remove, not empty\n");
        return 0;
    }
    srand((unsigned)time(0));
    int signature = rand()%INT_MAX + 1;
    setSignature(file,signature);
    setFreeBlock(file,FREE);
    //setIsDirectory(file,NOTDIRECTORY); // whenever we write to disk we will claim whether it's a directory so it's no need to set id here, besides, it can be used to undelete a directory;
    int i = 0;
    dd = getDD(file,&i);
    //printf("%i",i);
    dd->count--;
    memset(&dd->table[i],0,sizeof(FileDescriptor));
    write_blocks(dd->index,1,dd,map);
    for(int i = 0; i < NB_GARBAGE;i++) {
        if(garbage[i].index == 0) {
            strcpy(garbage[i].filename,fileName);
            garbage[i].index = file;
            garbage[i].directoryIndex = directory_cl->index;
            garbage[i].signature = signature;
            //printf("signature of file %i is %i:\n",file,signature);
            write_Garbage(i,garbage);
            break;
        }
    }
    return 1;
}
int rmfile(char* fileName) {
    int file = isFileExistD(fileName,directory_cl);
    if(!file) {
        perror("ERROR: file not exist\n");
        return 0;
    }
    if(id[file] == ISDIRECTORY) {
        perror("ERROR: it's a directory\n");
        return 0;
    }
    srand((unsigned)time(0));
    int signature = rand()%INT_MAX + 1;
    while(fat[file].next!=NO_NEXT){
        setFreeBlock(file,FREE);
        setSignature(file,signature);
    }
    setFreeBlock(file,FREE);
    setSignature(file,signature);
    int i = 0;
    DirectoryDescriptor *dd = getDD(file,&i);
    //printf("%i",i);
    memset(&dd->table[i],0,sizeof(FileDescriptor));
    dd->count--;
    write_blocks(dd->index,1,dd,map);
    for(int i = 0; i < NB_GARBAGE;i++) {
        if(garbage[i].index == 0) {
            strcpy(garbage[i].filename,fileName);
            garbage[i].index = file;
            garbage[i].directoryIndex = directory_cl->index;
            garbage[i].signature = signature;
            write_Garbage(i,garbage);
            break;
        }
    }
    return 1;
}
int rmForce(char * fileName) {
    if(!strcmp(fileName,".") || !strcmp(fileName,"..")){
        printf("ERROR: can't remove '%s':invalid argument\n",fileName);
    }
    int file = isFileExistD(fileName,directory_cl);
    if (!file) {
        perror("ERROR: file not exist");
        return 0;
    }
    if (file == ROOT){
        perror("ERROR: can't rm root");
        return 0;
    }
    if(id[file] == NOTDIRECTORY) {
        //printf("remove %s at %i",fileName,file);
        rmfile(fileName);
        return 1;
    }
    if(id[file] == ISDIRECTORY){
        DirectoryDescriptor * dd = (DirectoryDescriptor *)(map + file * BLOCK_SIZE);
        int* files = malloc(NB_BLOCK * sizeof(int));
        int fileNum = getFilesD(dd,files);
        if(fileNum == 2) {
            //printf("remove %s at %i",fileName,file);
            rmdirc(fileName);
            return 1;
        }
        cd(fileName);
        for(int i = 2; i < fileNum; i++) {
            rmForce(getFileName(dd,files[i]));

        }
        cd("..");
        //printf("remove %s at %i",fileName,file);
        rmdirc(fileName);
        free(files);
        return 1;
    }
    return 0;
}
int getNB(int * buffer) {
    for(int i = 0; i < NB_BLOCK; i++) {
        if(buffer[i]==0){
            return i;
        }
    }
    return 0;
}
void getAllocatedFiles(int file,int *blocks){
    //pwd();
    //printf("\n");
    if(id[file] == NOTDIRECTORY) {
        blocks[getNB(blocks)] = file;
        //char *fileName = getFileName(directory_cl, file);
        //printf("add file %s %i to blocksA\n",fileName , file);
        return;
    }
    if(id[file] == ISDIRECTORY){
        DirectoryDescriptor * dd = (DirectoryDescriptor *)(map + file * BLOCK_SIZE);
        int* files = malloc(NB_BLOCK * sizeof(int));
        int fileNum = getFilesD(dd,files);
        char *fileName = getFileName(directory_cl, file);
        //printf("file number in %s is %i",fileName,fileNum);
        if (fileNum == 2) {
            blocks[getNB(blocks)] = file;
            //printf("add file %s %i to blocksA\n", fileName, file);
            free(files);
            return;
        }
        if(file == ROOT) {
            for(int i = 2; i < fileNum; i++) {
                getAllocatedFiles(files[i],blocks);
            }
            blocks[getNB(blocks)] = file;
            //printf("add file %s %i to blocksA\n", fileName, file);
            free(files);
            return;
        }
        //printf("cd to %s",fileName);
        cd(fileName);
        for(int i = 2; i < fileNum; i++) {
            getAllocatedFiles(files[i],blocks);
        }
        cd("..");
        blocks[getNB(blocks)] = file;
        //printf("add file %s %i to blocksA\n", fileName, file);
        free(files);
        return;
    }
}
int cmp( const void *a , const void *b ){
return *(int *)a > *(int *)b ? 1 : -1;
}
int cmpfunc(const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}

int scandisk() {
    //get all allocated file's start block
    int * blocksA = malloc(sizeof(int) * NB_BLOCK);
    memset(blocksA, 0, sizeof(int) * NB_BLOCK);
    getAllocatedFiles(ROOT,blocksA);

    int size = getNB(blocksA);
    for(int i = 0 ; i < size; i++) {
        int tempb = blocksA[i];
        //printf("file %i:\n",tempb);
        while(fat[tempb].next!=NO_NEXT) {
            tempb = fat[tempb].next;
            blocksA[getNB(blocksA)] = tempb;
            //printf("      %i\n",tempb);
        }
    }
    qsort(blocksA,getNB(blocksA),sizeof(int),cmp);
    //get all BUSY blocks
    int * blocksB = malloc(sizeof(int) * NB_BLOCK);
    memset(blocksB, 0, sizeof(int) * NB_BLOCK);
    for (int i = ROOT; i < NB_BLOCK; i++) {
        if (fat[i].free == BUSY) {
            blocksB[getNB(blocksB)] = i;
        }
    }
    int i = 0;
    for(; i < getNB(blocksB); i++) {
        //printf("blocksB[%i] = %i\n",i,blocksB[i]);
    }
    qsort(blocksB,getNB(blocksB),sizeof(int),cmp);
    size = getNB(blocksA) < getNB(blocksB) ? getNB(blocksA) : getNB(blocksB);
    if(size == getNB(blocksA) && size == getNB(blocksB)){
        for(int i = 0; i < size; i++) {
            if(blocksA[i]!=blocksB[i]) {
                break;
            }
        }
    }
    if(i == size) {
        return 1;
    }
    for(int i = 0; i < getNB(blocksB); i++) {
        if(!bsearch(&blocksB[i], blocksA, getNB(blocksA), sizeof (int), cmpfunc)) {
            setFreeBlock(blocksB[i],FREE);
        }
    }
    for(int i = 0; i < getNB(blocksA); i++) {
        if(!bsearch(&blocksA[i], blocksB, getNB(blocksB), sizeof (int), cmpfunc)) {
            printf("page[%i] hasn't allocated yet, truncate  or allocate it\n",blocksA[i]);
        }
    }
    int j = 0;
    while(j < getNB(blocksA) - 1){
        int count = 1;
        while(blocksA[j] == blocksA[++j]) {
            count++;
        }
        if(count > 1) {
            printf("page[%i] allocated to %i files\n", blocksA[j-1], count);
        }
    }
    return 1;
}
int undelete(char * fileName) {
    int i = NB_GARBAGE - 1;
    for(; i >= 0; i--) {
        if(!strcmp(fileName,garbage[i].filename) && directory_cl->index == garbage[i].directoryIndex){
            break;
        }
    }
    //printf("found %i as fileIndex\n",garbage[i].index);
    if((i ==0 && !strcmp(fileName,garbage[0].filename) && directory_cl->index == garbage[0].directoryIndex) || i != 0){
        if(garbage[i].signature != fat[garbage[i].index].signature) {
            printf("Can't undelete\n");
            return 0;
        }
    } else {
        printf("Can't undelete\n");
        return 0;
    }
    int file = garbage[i].index;
    int tempf = file;
    //printf("%i can be recover\n",file);

    int signature = fat[tempf].signature;
    while (fat[tempf].next != NO_NEXT) {
        tempf = fat[tempf].next;
        if (fat[tempf].signature != signature) {
            printf("Can't undelete\n");
            return 0;
        }
    }
    tempf = file;
    fat[tempf].free = BUSY;
    fat[tempf].signature = 0;
    while(fat[tempf].next != NO_NEXT) {
        tempf = fat[tempf].next;
        fat[tempf].free = BUSY;
        fat[tempf].signature = 0;
    }


    //put file to directory_cl
    int blockStart = file;
    DirectoryDescriptor * tempD = directory_cl;

	while(tempD->count == 3) {
		int next = fat[tempD->index].next;
		if (next == NO_NEXT) {
            int directoryStart = createDirectoryDiscriptor(tempD);
			if(!directoryStart) {
                printf("Cant't undelete\n");
				return 0;
			} else {
			    tempD = (DirectoryDescriptor*)(map + directoryStart * BLOCK_SIZE);
			    //printf("\n%s %i\n","i get tempd",directoryStart);
				break;
			}

		}
		tempD = (DirectoryDescriptor*)(map + next*BLOCK_SIZE);
	}
    for(int i = 0; i < 3; i++) {
        if (!(*(char*)&(tempD->table[i]))) {
            strcpy(tempD->table[i].filename,fileName);
            tempD->table[i].index = blockStart;
            tempD->table[i].timestamp = time(NULL);
            tempD->count++;
            msync(map + (tempD->index) * BLOCK_SIZE, BLOCK_SIZE, MS_SYNC);
            break;
        }
    }
    return 1;
}

/*
 * filesystem() - loads in the filesystem and accepts commands
 */
void filesystem(char *myfilesystem)
{


	/* pointer to the memory-mapped filesystem */



	/*
	 * open file, handle errors, create it if necessary.
	 * should end up with map referring to the filesystem.
	 */
        f = fopen(myfilesystem,"r");
        if(!f) {
        f = fopen(myfilesystem,"w+");
		init_disk(f);
		fclose(f);
        fs = open(myfilesystem,O_RDWR);
		int SIZE = BLOCK_SIZE * NB_BLOCK;
		map = mmap(NULL,SIZE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,fs,0);
		init_fs(fat, root, id, garbage,map);
		close(fs);
        } else{
        fclose(f);
        }

		fs = open(myfilesystem,O_RDWR);
		int SIZE = BLOCK_SIZE * NB_BLOCK;
		map = mmap(NULL,SIZE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,fs,0);

        id = (int*)(map);
        fat = (FatNode*)(map + FILE_ALLOCATION_TABLE_START * BLOCK_SIZE);
        root = (DirectoryDescriptor*)(map + ROOT * BLOCK_SIZE);
        garbage = (Garbage*)(map + GARBAGE_START * BLOCK_SIZE);
        if(!verify_fs(fat, root, id, map)) {
            perror("ERROR: file system broken\n");
            exit(1);
        }else {
            directory_cl = root;
        }

	/* You will probably want other variables here for tracking purposes */


	/*
	 * Accept commands, calling accessory functions unless
	 * user enters "quit"
	 * Commands will be well-formatted.
	 */
	char *buffer = NULL;
	size_t size = 0;
	while(getline(&buffer, &size, stdin) != -1)
	{


		/* Basic checks and newline removal */
		size_t length = strlen(buffer);
		if(length == 0)
		{
			continue;
		}
		if(buffer[length-1] == '\n')
		{
			buffer[length-1] = '\0';
		}

		/* TODO: Complete this function */
		/* You do not have to use the functions as commented (and probably can not)
		 *	They are notes for you on what you ultimately need to do.
		 */

		if(!strcmp(buffer, "quit"))
		{
			break;
		}
		else if(!strncmp(buffer, "dump ", 5))
		{
			if(isdigit(buffer[5]))
			{
				dump(stdout, atoi(buffer + 5));
			}
			else
			{
				char *file = buffer + 5;
				char *space = strstr(buffer+5, " ");
				*space = '\0';
				//open and validate filename

				//validate filename
				if (sizeof(file) > 32) {
					perror("Error: invalid filename\n");
					continue;
				}
				if(!dumpBinary(file, atoi(space + 1))){
					perror("Error:dump fail\n");
				};
			}
		}
		else if(!strncmp(buffer, "usage", 5))
		{
			usage();
		}
		else if(!strncmp(buffer, "pwd", 3))
		{
			pwd();
		}
		else if(!strncmp(buffer, "cd ", 3))
		{
			cd(buffer+3);
		}
		else if(!strncmp(buffer, "ls", 2))
		{
			ls();
		}
		else if(!strncmp(buffer, "mkdir ", 6))
		{
			makedir(buffer+6);
		}
		else if(!strncmp(buffer, "cat ", 4))
		{
			cat(buffer + 4);
		}
		else if(!strncmp(buffer, "write ", 6))
		{
			char *filename = buffer + 6;
			char *space = strstr(buffer+6, " ");
			*space = '\0';
			size_t amt = atoi(space + 1);
            space = strstr(space + 1, " ");

			char *data = generateData(space+1, amt<<1);
			/*char * data = malloc(amt);
			for(int i = 0; i < amt; i++) {
                data[i] = 'h';
			}*/
			writef(filename, amt, data);
			free(data);
		}
		else if(!strncmp(buffer, "append ", 7))
		{

			char *filename = buffer + 7;
			char *space = strstr(buffer+7, " ");
			*space = '\0';
			size_t amt = atoi(space + 1);
            space = strstr(space + 1, " ");

			char *data = generateData(space+1, amt<<1);
			/*char * data = malloc(amt);
			for(int i = 0; i < amt; i++) {
                data[i] = 'o';
			}*/
			append(filename, amt, data);
			free(data);
		}
		else if(!strncmp(buffer, "remove ", 7))
		{
            char* file = buffer + 7;
            char *space = strstr(buffer+7," ");
            *space = '\0';
            size_t start = atoi(space + 1);
            space = strstr(space + 1, " ");
            *space = '\0';
            size_t end = atoi(space + 1);
			removef(file, start, end);
		}
		else if(!strncmp(buffer, "getpages ", 9))
		{
			getpages(buffer + 9);
		}
		else if(!strncmp(buffer, "get ", 4))
		{
			char *filename = buffer + 4;
			char *space = strstr(buffer+4, " ");
			*space = '\0';
			size_t start = atoi(space + 1);
			space = strstr(space+1, " ");
			*space = '\0';
			size_t end = atoi(space + 1);
			getf(filename, start, end);
		}
		else if(!strncmp(buffer, "rmdir ", 6))
		{
			rmdirc(buffer + 6);
		}
		else if(!strncmp(buffer, "rm -rf ", 7))
		{
			rmForce(buffer + 7);
		}
		else if(!strncmp(buffer, "rm ", 3))
		{
			rmfile(buffer + 3);
		}
		else if(!strncmp(buffer, "scandisk", 8))
		{
			scandisk();
		}
		else if(!strncmp(buffer, "undelete ", 9))
		{
			undelete(buffer + 9);
		}



		free(buffer);
		buffer = NULL;
	}
	free(buffer);
	buffer = NULL;

}

/*
 * help() - Print a help message.
 */
void help(char *progname)
{
	printf("Usage: %s [FILE]...\n", progname);
	printf("Loads FILE as a filesystem. Creates FILE if it does not exist\n");
	exit(0);
}

/*
 * main() - The main routine parses arguments and dispatches to the
 * task-specific code.
 */
int main(int argc, char **argv)
{
	/* for getopt */
	long opt;

	/* run a student name check */
	check_student(argv[0]);

	/* parse the command-line options. For this program, we only support */
	/* the parameterless 'h' option, for getting help on program usage. */
	while((opt = getopt(argc, argv, "h")) != -1)
	{
		switch(opt)
		{
		case 'h':
			help(argv[0]);
			break;
		}
	}
	if(argv[1] == NULL)
	{
		fprintf(stderr, "No filename provided, try -h for help.\n");
		return 1;
	}

	filesystem(argv[1]);

	return 0;
}
