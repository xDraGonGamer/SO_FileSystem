#include "../../fs/operations.h"
#include "../../fs/state.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define THREAD_COUNT 10
#define TEST_COUNT 5

void* threadfunc(void* filepath){
    char* filename = (char*) filepath;
    assert(tfs_open(filename, TFS_O_CREAT) != -1);
    return NULL;
}

int main(){
    pthread_t tid[THREAD_COUNT];
    char filepath[4] = "/f1";
    filepath[3] = '\0';
    assert(tfs_init() != -1);
    for(int i = 0; i < THREAD_COUNT; i++){
        assert(!pthread_create(&tid[i], 0, threadfunc, filepath));
    }
    for (int i=0; i<THREAD_COUNT; i++){
        pthread_join(tid[i], NULL);
    }

    assert(test1(filepath) == 1);

    printf("Successful test.\n");


    return 0;
}