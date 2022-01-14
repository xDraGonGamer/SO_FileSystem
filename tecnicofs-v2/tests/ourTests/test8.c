/**
 * test8.c
 * Testa se o programa consegue ler e escrever e fazer truncate sem erros em varios ficheiros, concurrentemente
 * 
 */

#include "../../fs/operations.h"
#include "../../fs/state.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>


#define TO_WRITE 20000
#define TO_READ 200
#define THREADS 40
#define OPEN_FILES 6
#define TEST_TIMES 10

void* truncateForThread(void* arg){
    int* number = (int*) arg;
    char path[5]="/fn\0\0";
    sprintf(&path[2],"%d",*number);
    int fhandle = tfs_open(path,TFS_O_TRUNC);
    assert(fhandle!=-1);
    assert(tfs_close(fhandle)!=-1);
    return NULL;
}

void* readForThread(void* arg){
    int* number = (int*) arg;
    char path[5]="/fn\0\0";
    char output[TO_READ];
    char c1='b';
    char c2= (char) ('a'+*number);
    char compare1[TO_READ];
    char compare2[TO_READ];
    memset(compare1, c1, TO_READ);
    memset(compare1, c2, TO_READ);
    sprintf(&path[2],"%d",*number);
    int fhandle = tfs_open(path,TFS_O_CREAT);
    assert(fhandle!=-1);
    ssize_t read = tfs_read(fhandle,output,TO_READ);
    assert( read==TO_READ || read==0);
    if (read){
        assert(memcmp(output,compare1,TO_READ)==0 || memcmp(output,compare2,TO_READ)==0);
    }
    assert(tfs_close(fhandle)!=-1);
    return NULL;
}

void* writeForThread(void* arg){
    int* number = (int*) arg;
    char path[5]="/fn\0\0";
    char writeChar = (char) ('a'+*number);
    char input[TO_WRITE];
    memset(input, writeChar, TO_WRITE);
    sprintf(&path[2],"%d",*number);
    int fhandle = tfs_open(path,TFS_O_APPEND);
    assert(fhandle!=-1);
    assert(tfs_write(fhandle,input,TO_WRITE)==TO_WRITE);
    assert(tfs_close(fhandle)!=-1);
    return NULL;
}

int main(){
    assert(OPEN_FILES<=20);
    pthread_t tid[THREADS];
    int fileNumbers[OPEN_FILES];
    char path[5]="/fn\0\0";
    char writeChar='t';
    char input[TO_WRITE];
    memset(input, writeChar, TO_WRITE);
    
    for (int n=0; n<TEST_TIMES ; n++){
    
        assert(tfs_init() != -1);

        for (int i=0;i<OPEN_FILES;i++){
            fileNumbers[i] = i;
            sprintf(&path[2],"%d",i);
            int fhandle = tfs_open(path,TFS_O_CREAT);
            assert(fhandle!=-1);
            assert(tfs_close(fhandle)!=-1);
        }

        int ind;
        for (int i=0; i<THREADS; i++){
            ind = i%OPEN_FILES;
            if ( !( (i%15)+1 ) ){
                assert(!pthread_create(&tid[i],0,truncateForThread,&fileNumbers[ind]));
            } else if (i%2){
                assert(!pthread_create(&tid[i],0,writeForThread,&fileNumbers[ind]));
            } else {
                assert(!pthread_create(&tid[i],0,readForThread,&fileNumbers[ind]));
            }
        }


        for (int i=0; i<THREADS; i++){
            pthread_join(tid[i], NULL);
        }

        tfs_destroy();

    }

    printf("Successful test.\n");

    return 0;

    


}