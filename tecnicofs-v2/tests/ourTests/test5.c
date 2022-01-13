/**
 * test5.c
 * Testa se o programa executa, sem problemas, quando são corridas threads em paralelo, 
 * executando tfs_close + tfs_write/tfs_read
 * 
 */
#include "../../fs/operations.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define F1_WRITE_SIZE 40000
#define F2_WRITE_SIZE 500
#define F2_INIT_SIZE 200000


typedef struct {
    int oldFhandle;
    char* newFile;
    char write[F2_WRITE_SIZE];
} closeStruct;

typedef struct {
    int fhandle;
    char write[F1_WRITE_SIZE];
} writeStruct;

void* closeForThread(void* cS){
    closeStruct* cStruct = (closeStruct*) cS;
    assert(tfs_close(cStruct->oldFhandle) != -1);
    int fhandle = tfs_open(cStruct->newFile,TFS_O_APPEND);
    assert(fhandle != -1);
    //assert(tfs_write(fhandle, cStruct->write, F2_WRITE_SIZE) == F2_WRITE_SIZE);
    assert(tfs_close(fhandle) != -1);
    //maybe check newFile content
    return NULL;
}

void* writeForThread(void* wS){
    char output[F1_WRITE_SIZE+1];
    writeStruct* wStruct = (writeStruct*) wS;
    ssize_t wrote = tfs_write(wStruct->fhandle, wStruct->write, F1_WRITE_SIZE);
    printf("Wrote = %ld\n",wrote);
    assert(wrote == F1_WRITE_SIZE);
    ssize_t read = tfs_read(wStruct->fhandle, output, F1_WRITE_SIZE);
    output[F1_WRITE_SIZE]='\0';
    printf("output = %s\n",output);
    assert(read==F1_WRITE_SIZE);
    assert(memcmp(wStruct->write,output,F1_WRITE_SIZE)==0);
    int result = tfs_close(wStruct->fhandle);
    if (result==-1){
        printf("Closed File on Close Thread\n");
    } else {
        printf("Closed in this Thread, INVALID TEST\n");
    }
    return NULL;
}


int main(){
    pthread_t tid[2];
    char file1[4] = "/f1\0";
    char file2[4] = "/f2\0";
    char f2Init[F2_INIT_SIZE];
    char f2BaseChar = 'j';
    char f1BaseChar = 'a';
    char f2AddChar = 'z';
    memset(f2Init,f2BaseChar,F2_INIT_SIZE);

    assert(tfs_init() != -1);
    //Init File2
    int fhandle = tfs_open(file2,TFS_O_CREAT);
    assert(fhandle!=-1);
    assert(tfs_write(fhandle, f2Init, F2_INIT_SIZE) == F2_INIT_SIZE);
    assert(tfs_close(fhandle) != -1);

    //Create File1
    fhandle = tfs_open(file1,TFS_O_CREAT);
    assert(fhandle!=-1);

    //Init write Struct
    writeStruct wStruct;
    wStruct.fhandle = fhandle;
    memset(wStruct.write,f1BaseChar,F1_WRITE_SIZE);

    //Init close Struct
    closeStruct cStruct;
    cStruct.oldFhandle = fhandle;
    cStruct.newFile = file2;
    memset(cStruct.write,f2AddChar,F2_WRITE_SIZE);

    printf("--------\n");

    assert(!pthread_create(&tid[0],0,writeForThread,&wStruct));
    assert(!pthread_create(&tid[1],0,closeForThread,&cStruct));

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);

    tfs_destroy();

    printf("Successful test.\n");

    return 0;

}