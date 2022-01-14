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

#define F1_WRITE_SIZE 50000
#define F2_INIT_SIZE 200000
#define TEST_TIMES 100



typedef struct {
    int oldFhandle;
    char* newFile;
} closeStruct;

typedef struct {
    int fhandle;
    inode_t* oldInode;
    char write[F1_WRITE_SIZE];
} writeStruct;

typedef struct {
    inode_t* inode;
    ssize_t written;
} threadReturn;


void checkIfValidOutput(threadReturn* tWrite, threadReturn* tClose){
    if (tWrite->written == -1){ 
        assert(tWrite->inode->i_size==0 && tClose->inode->i_size==F2_INIT_SIZE);
        /* "tfs_write returned -1, this can mean that it wasn't able to write because there were no
         * open_file_entries in the specified fhandle (it can happen in this test),
         * or it has another error.
         * tfs_write has been tested and it seems to be working perfectly, furthermore, while developing
         * this tests, some control checks were added to the tfs functions and they always showed that
         * tfs_write would only return -1 in this test when there were no
         * open_file_entries in the specified fhandle*/
    } else {
        assert(tWrite->written==F1_WRITE_SIZE);
        //Case1: It wrote on file 2, having closed file 1 before
        char cond1 = (tWrite->inode->i_size==0 && tClose->inode->i_size==(F2_INIT_SIZE+F1_WRITE_SIZE));
        //Case2: Wrote on file1
        char cond2 = (tWrite->inode->i_size==F1_WRITE_SIZE && tClose->inode->i_size==F2_INIT_SIZE);

        assert(cond1 || cond2);
    }
}



void* closeForThread(void* cS){
    closeStruct* cStruct = (closeStruct*) cS;
    threadReturn* tRet = (threadReturn*) malloc(sizeof(threadReturn));
    assert(tfs_close(cStruct->oldFhandle) != -1);
    int fhandle = tfs_open(cStruct->newFile,TFS_O_APPEND);
    assert(fhandle != -1);
    open_file_entry_t* entry2 = get_open_file_entry(fhandle);
    tRet->inode = inode_get(entry2->of_inumber);
    assert(tfs_close(fhandle) != -1);
    return tRet;
}

void* writeForThread(void* wS){
    writeStruct* wStruct = (writeStruct*) wS;
    threadReturn* tRet = (threadReturn*) malloc(sizeof(threadReturn));
    ssize_t wrote = tfs_write(wStruct->fhandle, wStruct->write, F1_WRITE_SIZE);
    tRet->written = wrote;
    tRet->inode = wStruct->oldInode;
    return tRet;
}


int main(){
    pthread_t tid[2];
    char file1[4] = "/f1\0";
    char file2[4] = "/f2\0";
    char f2Init[F2_INIT_SIZE];
    char f2BaseChar = 'j';
    char f1BaseChar = 'a';
    memset(f2Init,f2BaseChar,F2_INIT_SIZE);

    for (int i=0; i<TEST_TIMES;i++){
        assert(tfs_init() != -1);
        //Init File2
        int fhandle = tfs_open(file2,TFS_O_CREAT);
        assert(fhandle!=-1);
        assert(tfs_write(fhandle, f2Init, F2_INIT_SIZE) == F2_INIT_SIZE);
        assert(tfs_close(fhandle) != -1);


        //Create File1
        fhandle = tfs_open(file1,TFS_O_CREAT);
        assert(fhandle!=-1);
        
        //Structs which have threads "returns"
        void* tW;
        void* tC;
        
        //Init write Struct
        writeStruct wStruct;
        wStruct.fhandle = fhandle;
        memset(wStruct.write,f1BaseChar,F1_WRITE_SIZE);
        open_file_entry_t *oldFile = get_open_file_entry(fhandle);
        wStruct.oldInode = inode_get(oldFile->of_inumber);

        //Init close Struct
        closeStruct cStruct;
        cStruct.oldFhandle = fhandle;
        cStruct.newFile = file2;

        assert(!pthread_create(&tid[0],0,writeForThread,&wStruct));
        assert(!pthread_create(&tid[1],0,closeForThread,&cStruct));

        pthread_join(tid[0], &tW);
        pthread_join(tid[1], &tC);

        threadReturn* tWrite = (threadReturn*) tW;
        threadReturn* tClose = (threadReturn*) tC;

        checkIfValidOutput(tWrite,tClose);


        tfs_destroy();
    }

    printf("Successful test.\n");

    return 0;

}

