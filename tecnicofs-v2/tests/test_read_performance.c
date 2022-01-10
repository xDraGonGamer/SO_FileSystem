#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define SIZE 10
#define COUNT 1000



void* readForThread(void* entry){
    int* f = (int*) entry;
    char input[SIZE+1];
    //size_t offset = get_open_file_entry(*f)->of_offset;
    memset(input, 'A', SIZE);
    char output [SIZE + 1];
    input[SIZE]='\0'; output[SIZE]='\0';
    assert(tfs_read(*f, output, SIZE)==SIZE);
    printf("Output:%s\n\n",output);
    /*if (memcmp(input, output, SIZE)!=0){
        printf("Input:%s\nOutput:%s\n\n",input,output);
        printf("OldOffset=%ld\n",offset);
        printf("NewOffset=%ld\n",get_open_file_entry(*f)->of_offset);
        printf("%ld\tZeroCheck = %d\n",strlen(output),output[strlen(output)]=='\0');
    }*/
    assert(memcmp(input, output, SIZE) == 0);
    return NULL;
}



int main() {
    pthread_t tid[COUNT];
    char *path = "/f1";
    char input[SIZE+1];
    clock_t startSeq, endSeq, startThread, endThread;
    memset(input, 'A', SIZE);

    char output [SIZE + 1];

    assert(tfs_init() != -1);

    int f;

    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);
    int me = 'A';
    for (int i = 0; i < COUNT; i++) {
        memset(input, me, SIZE);
        assert(tfs_write(f, input, SIZE) == SIZE);
        input[SIZE] = '\0';
        me++;
        me = 'A' + (me%20);
    }
    assert(tfs_close(f) != -1);


    // Teste Sequencial

    f = tfs_open(path, 0);
    assert(f != -1);
    me = 'A';
    startSeq = clock();
    for (int i = 0; i < COUNT; i++) {
        size_t offset = get_open_file_entry(f)->of_offset;
        assert(tfs_read(f, output, SIZE)==SIZE);
        memset(input, me, SIZE);
        if (memcmp(input, output, SIZE)){
            input[SIZE] = '\0';
            output[SIZE] = '\0';
            printf("OldOffset:%ld\nNewOff:%ld\n",offset, get_open_file_entry(f)->of_offset);
            printf("Input:%s\nOutput:%s\n\n",input,output);
        }
        assert(memcmp(input, output, SIZE) == 0);
        me++;
        me = 'A' + (me%20);
    }
    endSeq = clock();
    assert(tfs_close(f) != -1);

    // Teste multi-thread

    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    startThread = clock();
    for (int i=0 ; i<COUNT ; i++){
        assert(!pthread_create(&tid[i],0,readForThread,&f));
    }

    for (int i=0; i<COUNT ; i++){
        pthread_join(tid[i], NULL);
    }
    
    endThread = clock();
    assert(tfs_close(f) != -1);

    printf("Successful test.\n");

    clock_t Seq, Thread;
    Seq = endSeq - startSeq;
    Thread = endThread - startThread;

    printf("Thread: %ld\tSeq: %ld\n",Thread,Seq);
    printf("Programa em paralelo foi %ldx mais rápido\n",Seq/Thread);

    return 0;
}
