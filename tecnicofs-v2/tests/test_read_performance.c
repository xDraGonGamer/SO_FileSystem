#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define MAX_FILE_NAME_SIZE 3
#define FILES 4
#define SIZE 200000



void* readForThread(void* fileNumber){
    int* number = (int*) fileNumber;
    char output [SIZE];
    char path[MAX_FILE_NAME_SIZE+1];
    path[0]='/';
    sprintf(path+1,"%d",*number);
    int f;
    f = tfs_open(path, TFS_O_CREAT);
    ssize_t read = tfs_read(f, output, SIZE);
    assert(read==SIZE);
    assert(tfs_close(f) != -1);
    return NULL;
}

void readForSeq(int number){
    char output[SIZE];
    char path[MAX_FILE_NAME_SIZE+1];
    path[0]='/';
    sprintf(path+1,"%d",number);
    int f;
    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_read(f, output, SIZE)==SIZE);
    assert(tfs_close(f) != -1);
}



int main() {
    pthread_t tid[FILES];
    struct timespec startS, finishS, startT, finishT;
    __time_t seqTime,threadTime;
    int* fileNumbers = (int*) malloc(sizeof(int)*FILES);
    assert(tfs_init() != -1);
    char path[MAX_FILE_NAME_SIZE+1];
    path[0]='/';
    int f;
    char input[SIZE];

    for (int i=0;i<FILES;i++){
        fileNumbers[i] = i;
    }    

    for (int i=0;i<FILES;i++){
        sprintf(path+1,"%d",i);
        f = tfs_open(path, TFS_O_CREAT);
        assert(f != -1);
        int me = 'A';
        memset(input, me, SIZE);
        assert(tfs_write(f, input, SIZE) == SIZE);
        assert(tfs_close(f) != -1);
    }
    
    // Test Sequential
    clock_gettime(CLOCK_MONOTONIC, &startS);
    for (int i=0;i<FILES;i++){
        readForSeq(i);
    }
    clock_gettime(CLOCK_MONOTONIC, &finishS);
    seqTime = (finishS.tv_nsec - startS.tv_nsec);
    // Teste multi-thread


    clock_gettime(CLOCK_MONOTONIC, &startT);
    for (int i=0 ; i<FILES ; i++){
        assert(!pthread_create(&tid[i],0,readForThread,&fileNumbers[i]));
    }

    for (int i=0; i<FILES ; i++){
        pthread_join(tid[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &finishT);
    threadTime = (finishT.tv_nsec - startT.tv_nsec);

    printf("Successful test.\n");
    printf("Programa em paralelo foi %ldx mais rápido\n\n",seqTime/threadTime);
    printf("Time in nanoseconds:\nSequential: %ld ns\tThread: %ld ns\n",seqTime,threadTime);

    return 0;
}
