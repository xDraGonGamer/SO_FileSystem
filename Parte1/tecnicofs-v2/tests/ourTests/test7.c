/**
 * test7.c
 * Testa se o programa consegue escrever em MAX_OPEN_FILES ficheiros TO_WRITE chars, concurrentemente, 
 * sabendo que vao haver ficheiros que vao escrever menos, porque TO_WRITE*MAX_OPEN_FILES > FS_SIZE 
 * 
 * Às vezes falha, mas é suposto nessa situação. Poderermos explicar na discussão
 * 
 */

#include "../../fs/operations.h"
#include "../../fs/state.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define FS_SIZE (BLOCK_SIZE * DATA_BLOCKS)
#define TO_WRITE ((FS_SIZE - (BLOCK_SIZE*(MAX_OPEN_FILES-1))) / (MAX_OPEN_FILES-1))
#define DIRECT_SIZE (BLOCK_SIZE*DIRECT_BLOCK_COUNT)
#define TEST_TIMES 20

typedef struct {
    int fileNumber;
    ssize_t written;
} writeStruct;


void checkValidWriteValues(writeStruct* writeS){
    size_t expectedSize = FS_SIZE;
    expectedSize -= BLOCK_SIZE; //bloco da raíz
    ssize_t writeCount=0;
    ssize_t written;
    for (int i=0; i<MAX_OPEN_FILES; i++){
        written = writeS[i].written;
        if (written>=0){
            if (written>DIRECT_SIZE){
                expectedSize -= BLOCK_SIZE; //bloco de indirecao
            }
            if (written == TO_WRITE){
                // retirar o que pode nao preencher se (TO_WRITE%BLOCK_SIZE)!=0
                expectedSize -= (BLOCK_SIZE - (TO_WRITE%BLOCK_SIZE));
            }
            writeCount += writeS[i].written;
        }
    }
    if (writeCount!=expectedSize){
        printf("wrote:%ld,expected:%ld\n",writeCount,expectedSize);
    }
    assert(writeCount==expectedSize);
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
    assert(wStruct->written!=-1);
    assert(tfs_close(f) != -1);
    return NULL;
}

void* readForThread(void* arg){
    writeStruct* wStruct = (writeStruct*) arg;
    ssize_t wrote = wStruct->written;
    char writeChar = 'a';
    char path[5]="/fn\0\0";
    sprintf(&path[2],"%d",wStruct->fileNumber);
    char output[wrote+1];
    char compare[TO_WRITE];
    memset(compare, writeChar, TO_WRITE);
    compare[(wrote-1)] ='\0';
    int f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_read(f, output, TO_WRITE)==wrote);
    output[(wrote-1)] ='\0';
    assert(strcmp(output,compare)==0);
    return NULL;
}


int main(){
    pthread_t tid[MAX_OPEN_FILES];
    writeStruct wSs[MAX_OPEN_FILES];
    for (int n=0; n<TEST_TIMES; n++){

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


        for (int i=0 ; i<MAX_OPEN_FILES ; i++){
            assert(!pthread_create(&tid[i],0,readForThread,&wSs[i]));
        }  

        for (int i=0 ; i<MAX_OPEN_FILES ; i++){
            pthread_join(tid[i], NULL);
        }    

        
        

        tfs_destroy();
    }

    printf("Successful test.\n");

    return 0;

}
    
