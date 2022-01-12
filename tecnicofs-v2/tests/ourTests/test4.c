/**
 * test4.c
 * Testa se o programa executa, sem problemas, quando são corridas threads em paralelo, executando umas 
 * o tfs_write e outra o tfs_open com a flag 'TFS_O_TRUNC'
 * 
 */

#include "../../fs/operations.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define SIZE 2000
#define THREADS 20
#define TEST_TIMES 5

typedef struct {
    char* filePath;
    char writeChar;
} writeStruct;


typedef struct {
    char* charsPossiblyWrittten;
    int charsStillAvailable;
} helper;

static char BASE_CHAR = 'a';


void* writeForThread(void* wS){
    writeStruct* wStruct = (writeStruct*) wS;
    char input[SIZE];
    int fhandle = tfs_open(wStruct->filePath,TFS_O_APPEND);
    assert(fhandle != -1);
    memset(input, wStruct->writeChar, SIZE);
    assert(tfs_write(fhandle, input, SIZE) == SIZE);
    assert(tfs_close(fhandle) != -1);
    return NULL;
}

void* truncateForThread(void* prePath){
    char* path = (char*) prePath;
    int fhandle = tfs_open(path,TFS_O_TRUNC);
    assert(fhandle != -1);
    assert(tfs_close(fhandle) != -1);
    return NULL;
}

helper* initHelper(){
    helper* help = (helper*) malloc(sizeof(helper));
    help->charsPossiblyWrittten = (char*) malloc((THREADS-1));
    for (int i=0;i<(THREADS-1);i++){
        help->charsPossiblyWrittten[i] = (char) (BASE_CHAR+i);
    }
    help->charsStillAvailable = THREADS-1;
    return help;
}

int findInHelper(helper* help, char* output, int start){
    int n=0,i=0;
    char input[SIZE];
    char curr;
    while (n<help->charsStillAvailable){
        if ((curr=(help->charsPossiblyWrittten[i])) != '\0'){
            if (curr == output[start]){
                memset(input,curr,SIZE);
                return memcmp(output,input,SIZE);
            } else {
                n++;
            }
        }
        i++;
    }
    return -1;
}

void isReadOk(ssize_t readChars,char* output){
    // será readChars um número válido?
    long int loops;
    assert((readChars>=0) && ((loops=(readChars%SIZE))==0) && (readChars<=(SIZE*(THREADS-1))));

    helper* help = initHelper();
    for (int i=0; i<loops; i++){
        assert(findInHelper(help,output,(i*SIZE))==0);
    }
    
}


int main(){
    pthread_t tid[THREADS];
    writeStruct arr[(THREADS-1)];
    char path[4] = "/f1\0";
    //Criar o ficheiro
    assert(tfs_init() != -1);
    int fhandle = tfs_open(path,TFS_O_CREAT);
    assert(fhandle!=-1);
    assert(tfs_close(fhandle) != -1);

    for (int t=0; t<TEST_TIMES; t++){

        int truncateThread = (( rand() )%THREADS);

        for (int i=0;i<(THREADS-1);i++){
            arr[i].writeChar = (char) (BASE_CHAR+i);
            arr[i].filePath = path;
        }
        int i=0;
        int arrInd=0;
        while(i<THREADS){
            if (i==truncateThread){
                assert(!pthread_create(&tid[i],0,truncateForThread,path));
            } else {
                assert(!pthread_create(&tid[i],0,writeForThread,&arr[arrInd]));
                arrInd++;
            }
            i++;
        }
        for (int n=0; n<THREADS ; n++){
            pthread_join(tid[n], NULL);
        }
        // Reading
        size_t maxSize = (THREADS-1)*SIZE;
        char output[maxSize];
        fhandle = tfs_open(path,TFS_O_CREAT);
        assert(fhandle!=-1);
        ssize_t read = tfs_read(fhandle, output, maxSize);

        // Check if reading output is one of the possible results
        isReadOk(read,output);
        //printf("output=%s,len=%ld\n",output,read);
    }
    printf("Successful test.\n");
    

    return 0;
}