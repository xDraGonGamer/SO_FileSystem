/**
 * test7.c
 * Testa se o programa consegue escrever em MAX_OPEN_FILES ficheiros, concurrentemente, sabendo que o último
 * write vai exceder o espaço disponível no fs, devendo escrever apenas o que consegue
 * 
 */

#include "../../fs/operations.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define FS_SIZE (BLOCK_SIZE * DATA_BLOCKS)
#define TO_WRITE ((FS_SIZE - (BLOCK_SIZE*(MAX_OPEN_FILES-1))) / (MAX_OPEN_FILES-1))

typedef struct {
    int fileNumber;
    ssize_t written;
} writeStruct;



void checkValidWriteValues(writeStruct* writeS){
    ssize_t writeCount=0;
    for (int i=0; i<MAX_OPEN_FILES; i++){
        printf("wrote = %ld\n",writeS[i].written);
        if (writeS[i].written>=0){
            writeCount += writeS[i].written;
        }
    }
    printf("wCount:%ld,FS_SIZE:%d\n",writeCount,FS_SIZE);
    assert(writeCount==FS_SIZE);
}

void* writeForThread(void* arg){
    writeStruct* wStruct = (writeStruct*) arg;
    char writeChar = 'a';
    char path[5]="/fn\0\0";
    char input[TO_WRITE];
    memset(input, writeChar, TO_WRITE);
    sprintf(&path[2],"%d",wStruct->fileNumber);
    int f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);
    wStruct->written = tfs_write(f, input, TO_WRITE);
    printf("written:%ld\n",wStruct->written);
    assert(tfs_close(f) != -1);
    return NULL;
}


int main(){
    pthread_t tid[MAX_OPEN_FILES];
    writeStruct wSs[MAX_OPEN_FILES];
    /*size_t toWrite = ((FS_SIZE - (BLOCK_SIZE*(MAX_OPEN_FILES-1))) / (MAX_OPEN_FILES-1));
    if (!( (FS_SIZE - (BLOCK_SIZE*(MAX_OPEN_FILES-1))) % (MAX_OPEN_FILES-1))){
        toWrite--;
    }*/

    assert(tfs_init() != -1);

    for (int i=0;i<MAX_OPEN_FILES;i++){
        wSs[i].fileNumber = i;
    }

    for (int i=0 ; i<MAX_OPEN_FILES ; i++){
        assert(!pthread_create(&tid[i],0,writeForThread,&wSs[i]));
    }  

    for (int i=0 ; i<MAX_OPEN_FILES ; i++){
        pthread_join(tid[i], NULL);
    }    


    checkValidWriteValues(wSs);

    tfs_destroy();

    printf("Successful test.\n");

    return 0;

}
    
