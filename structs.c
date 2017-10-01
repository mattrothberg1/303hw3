#include"structs.h"

/*
 *
 * Definition of structs in structs.h
 *
 */

int write_blocks(int start,int end,void *buffer){
    
}
int init_disk(){
    char buffer[BLOCK_SIZE] = 0;
    for (int x = 0; x < NB_PAGE; x++) {
        write_blocks(x, x+1, buffer);
    }
}
