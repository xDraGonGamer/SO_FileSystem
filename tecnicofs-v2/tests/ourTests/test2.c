/**
 * test2.c
 * Testa se o tfs_read tem comportamento sequencial quando chamado concurrentemente,
 * para o mesmo open_file_entry.
 * 
 * Para aumentar a probabilidade de aparecerem erros, caso houvesse algum, o teste
 * é repetido TEST_TIMES vezes
 * 
 */

#include "../../fs/operations.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>


#define COUNT 20
#define CHAR_COUNT 20
#define TEST_TIMES 20

static char BASE_CHAR='a';

typedef struct {
    int fhandle;
    int readCheck[COUNT];
} test2Struct;


char validOutput(char* output){
    char writingChar;
    char compareStr[CHAR_COUNT];
    for (char i=0; i<COUNT;i++){
        writingChar = (char) (BASE_CHAR + i);
        memset(compareStr, writingChar, CHAR_COUNT);
        if (!memcmp(output,compareStr,CHAR_COUNT)){
            return 0;
        }
    }
    return -1;
}


void* readForThread(void* t2StructVoid){
    test2Struct* t2Struct = (test2Struct*) t2StructVoid;
    int fhandle = t2Struct->fhandle;
    int ind; 
    char output[CHAR_COUNT];
    assert(tfs_read(fhandle, output, CHAR_COUNT)==CHAR_COUNT);
    assert(validOutput(output)==0);
    ind = output[0]-BASE_CHAR;
    t2Struct->readCheck[ind] = 1;

    return NULL;
}





int main(){
    pthread_t tid[COUNT];
    char input[CHAR_COUNT];
    char writingChar;
    char filePath[4] = "/f1";
    filePath[3] = '\0';
    int fhandle;
    assert(tfs_init() != -1);
    fhandle = tfs_open(filePath,TFS_O_CREAT);
    assert(fhandle != -1);

    // Writing
    for (char i=0; i<COUNT;i++){
        writingChar = (char) (BASE_CHAR + i);
        memset(input, writingChar, CHAR_COUNT);
        assert(tfs_write(fhandle, input, CHAR_COUNT) == CHAR_COUNT);
    }
    assert(tfs_close(fhandle) != -1);


    // Reading
    test2Struct* t2Struct = (test2Struct*) malloc(sizeof(test2Struct));

    for (int test=0; test<TEST_TIMES; test++){

        fhandle = tfs_open(filePath,TFS_O_CREAT);
        assert(fhandle != -1);
        t2Struct->fhandle = fhandle;

        //Init readCheck
        for (int i=0; i<COUNT; i++){
            t2Struct->readCheck[i]=0;
        }

        for (int i=0 ; i<COUNT ; i++){
            assert(!pthread_create(&tid[i],0,readForThread,t2Struct));
        }

        for (int i=0; i<COUNT ; i++){
            pthread_join(tid[i], NULL);
        }
        assert(tfs_close(fhandle) != -1);
    }

    // Checking if tfs_read read all expected values
    for (int i=0; i<COUNT; i++){
        assert(t2Struct->readCheck[i]==1);
    }


    printf("Successful test.\n");

    return 0;

}