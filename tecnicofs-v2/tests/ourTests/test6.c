/**
 * test6.c
 * Testa se o programa consegue escrever e ler tipos diferentes de "char"
 * (Não é multi-thread, foi apenas para testar)
 * 
 */

#include "../../fs/operations.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>


#define SIZE 20

int main(){

    int input[20];
    //init Input
    for (int i=0; i<SIZE; i++){
        input[i] = i;
    }

    int output[20];
    char file[4] = "/f1\0";

    assert(tfs_init() != -1);
    int fhandle = tfs_open(file,TFS_O_CREAT);

    assert(tfs_write(fhandle,input, sizeof(int)*SIZE) == sizeof(int)*SIZE);
    assert(tfs_close(fhandle) != -1);

    fhandle = tfs_open(file,TFS_O_CREAT);
    assert(tfs_read(fhandle,output,sizeof(int)*SIZE) == sizeof(int)*SIZE);
    assert(tfs_close(fhandle) != -1);
    for (int i=0; i<SIZE; i++){
        assert(output[i]==i);
    }

    tfs_destroy();

    printf("Successful test.\n");
    return 0;

}