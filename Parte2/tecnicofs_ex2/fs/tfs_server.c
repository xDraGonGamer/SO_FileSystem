#include "operations.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#define INPUT_BUFFER_SIZE 1041 //buffer for info from client
#define MAX_THREAD_INPUT_SIZE 2000 //TODO
#define BUFFER_PARTS 4


typedef struct {
    char readable;
    char info[MAX_THREAD_INPUT_SIZE];
} bufferPC;


typedef struct {
    int sessionID;
    size_t clientInfoMaxSize;
} clientRequestInfo;


bufferPC threadBuffers[S]; //consumer-producer buffers for every thread
int clientsFHandle[S]; //client table


pthread_cond_t sessionsCondVars[S];
pthread_mutex_t sessionsMutexes[S]; 
pthread_mutex_t writingMutex;
pthread_mutex_t openClientSessionMutex;



char initPthreadVars(){
    for (int i=0; i<S; i++){
        if ( (pthread_cond_init(&sessionsCondVars[i],NULL) < 0)
        || (pthread_mutex_init(&sessionsMutexes[i],NULL) < 0) ){
            return -1;
        }
    }
    return 0;
}

void initClientsFHandle(){
    pthread_mutex_init(&openClientSessionMutex, NULL);
    for (int i=0; i<S; i++){
        clientsFHandle[i] = -1;
    }
}

int getClientFhandle(int sessionId){
    if (sessionId >= S){
        return -1;
    } else {
        return clientsFHandle[sessionId];
    }
}

int openClientSession(char* client_pipe_name, int* fhandle){
    // maybe lock? TODO
    pthread_mutex_lock(&openClientSessionMutex);
    for (int i=0; i<S; i++){
        if (clientsFHandle[i]<0){
            *fhandle = open(client_pipe_name,O_WRONLY); //abre se o pipe
            if (*fhandle >= 0){
                clientsFHandle[i] = *fhandle;
                pthread_mutex_unlock(&openClientSessionMutex);
                return i;
            } else {
                pthread_mutex_unlock(&openClientSessionMutex);
                return -1;
            }
        }
    }
    pthread_mutex_unlock(&openClientSessionMutex);
    *fhandle = open(client_pipe_name,O_WRONLY); //ver isto depois
    return -1;
}

int finishClientSession(int sessionId){
    if (sessionId >= S){
        return -1;
    } 
    //TODO maybe lock////////////////
    clientsFHandle[sessionId] = -1;
    return 0;

}


char handle_tfs_mount(char *bufferIn){
    char bufferOut[sizeof(int)];
    char client_pipe_name[40];
    int out, fclient = -1;
    strcpy(client_pipe_name,bufferIn);
    out = openClientSession(client_pipe_name,&fclient);
    if(fclient < 0){
        return -1;
    }
    memcpy(bufferOut,&out,sizeof(int));
    if (write(fclient,bufferOut,sizeof(int)) < 0){
        return -1;
    } return 0;
}

char handle_tfs_unmount(char *bufferIn){
    char bufferOut[sizeof(int)];
    int clientSessionID, fclient, out;
    ssize_t ret;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    out = 0;
    fclient = getClientFhandle(clientSessionID);
    if (fclient<0 || finishClientSession(clientSessionID)<0){
        out = -1;
    }
    // TODO: Falta meter o clientsFHandle[SessionID] a -1, para saber que ha erro
    memcpy(bufferOut,&out,sizeof(int));
    ret = write(fclient,bufferOut,sizeof(int));
    if (ret>=0){
        if (close(fclient)<0){
            return -1;
        }
        return 0;
    }
    return -1;
}

char handle_tfs_open(char* bufferIn){
    char bufferOut[sizeof(int)];
    char fileName[41];
    int clientSessionID, fclient, flags, out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    strcpy(fileName,&bufferIn[4]);
    memcpy(&flags,&bufferIn[4 + strlen(fileName) + 1],sizeof(int));
    out = tfs_open(fileName,flags);

    memcpy(bufferOut,&out,sizeof(int));
    if (write(fclient,bufferOut,sizeof(int))){
        return -1;
    } return 0;
}

char handle_tfs_close(char* bufferIn){
    char bufferOut[sizeof(int)];
    int clientSessionID, fclient, fhandle, out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    memcpy(&fhandle,&bufferIn[4],sizeof(int));
    out = tfs_close(fhandle);

    memcpy(bufferOut,&out,sizeof(int));
    if (write(fclient,bufferOut,sizeof(int)) < 0){
        return -1;
    } return 0;
}

char handle_tfs_write(char* bufferIn){
    char bufferOut[sizeof(ssize_t)];
    char* toWrite;
    int clientSessionID, fclient, fhandle, out;
    size_t len;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    memcpy(&fhandle,&bufferIn[4],sizeof(int));
    memcpy(&len,&bufferIn[8],sizeof(size_t));
    toWrite = (char*) malloc(sizeof(char)*len); 
    memcpy(toWrite,&bufferIn[16],len);
    out = (int) tfs_write(fhandle,toWrite,len);
    free(toWrite);
    memcpy(bufferOut,&out,sizeof(int));
    if (write(fclient,bufferOut,sizeof(ssize_t)) < 0){
        return -1;
    } return 0;
}

char handle_tfs_read(char* bufferIn){
    int clientSessionID, fclient, fhandle, out;
    size_t len;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    memcpy(&fhandle,&bufferIn[4],sizeof(int));
    memcpy(&len,&bufferIn[8],sizeof(size_t));
    char bufferOut[sizeof(char)*len + sizeof(ssize_t)];
    out = (int) tfs_read(fhandle,&bufferOut[sizeof(ssize_t)],len);
    memcpy(bufferOut,&out,sizeof(int));
    if (write(fclient,bufferOut,sizeof(char)*len + sizeof(ssize_t)) < 0){
        return -1;
    } return 0;
}


char handle_tfs_shutdown_after_all_closed(char* bufferIn){
    char bufferOut[sizeof(int)];
    int clientSessionID, fclient, out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    out = tfs_destroy_after_all_closed();

    memcpy(bufferOut,&out,sizeof(int));
    if (write(fclient,bufferOut,sizeof(int)) < 0){
        return -1;
    } return 0;
}

clientRequestInfo getClientRequestInfo(char opCode, char* bufferFromClient){
    clientRequestInfo clientInfo;
    if (opCode==1){
        clientInfo.sessionID = -1;
    } else {
        memcpy(&(clientInfo.sessionID),&bufferFromClient[1],sizeof(int));
        switch (opCode){
            case 2:
                clientInfo.clientInfoMaxSize = 5;
                break;
            case 3:
                clientInfo.clientInfoMaxSize = 49;
                break;
            case 4:
                clientInfo.clientInfoMaxSize = 9;
                break;
            case 5:
                clientInfo.clientInfoMaxSize = 1041;
                break;
            case 6:
                clientInfo.clientInfoMaxSize = 17;
                break;
            default: // case 7
                clientInfo.clientInfoMaxSize = 5;
        }
    }
    return clientInfo;
}


void writeInBufferPC(char* bufferIn, clientRequestInfo clientInfo){
    memcpy(threadBuffers[clientInfo.sessionID].info,bufferIn,clientInfo.clientInfoMaxSize);
    threadBuffers[clientInfo.sessionID].readable = 1;
    
}

// código para a thread que recebe info dos clientes
void* threadReceiver(void* server_pipe_name){
    pthread_mutex_init(&writingMutex,NULL);
    char* pipename = (char*) server_pipe_name;
    ssize_t readOut;
    clientRequestInfo clientInfo;
    char bufferIn[INPUT_BUFFER_SIZE];
    char opCode;
    if(mkfifo(pipename,0777) < 0){
        exit(1);
    }

    int fserver = open(pipename, O_RDONLY);
    if(fserver < 0){
        //TODO struct for error, or exit(error) (algo assim)
    }
    while (1){
        readOut = read(fserver, bufferIn, INPUT_BUFFER_SIZE);
        if (!readOut){
            fserver = open(pipename, O_RDONLY);
            if(fserver < 0){
                //TODO struct for error, or exit(error) (algo assim)
            }
        } else if (readOut == -1) { //TODO, just here for debuging (maybe)
            fprintf(stderr, "[ERR]: read failed\n");
            exit(EXIT_FAILURE);
        } else {
            memcpy(&opCode,bufferIn,sizeof(char));
            clientInfo = getClientRequestInfo(opCode,bufferIn);
            if (clientInfo.sessionID < 0){
                //TODO: foi chamado um mount
            } else {
                writeInBufferPC(bufferIn, clientInfo);
                pthread_cond_signal(&sessionsCondVars[clientInfo.sessionID]);
            }
        }
    }
}

void readFromBufferPC(char* buffer, int sessionID){
    memcpy(buffer,threadBuffers[sessionID].info,1041);
    threadBuffers[sessionID].readable = 0;
}



char runClientRequest(char* info){
    char opCode, handleOut;
    memcpy(&opCode,info,sizeof(char));
    switch (opCode) {
        case 1:
            handleOut = handle_tfs_mount(&info[1]);
            break;
        case 2:
            handleOut = handle_tfs_unmount(&info[1]);
            break;
        case 3:
            handleOut = handle_tfs_open(&info[1]);
            break;
        case 4:
            handleOut = handle_tfs_close(&info[1]);
            break;
        case 5:
            handleOut = handle_tfs_write(&info[1]);
            break;
        case 6:
            handleOut = handle_tfs_read(&info[1]);
            break;
        default: // case 7
            handleOut = handle_tfs_shutdown_after_all_closed(&info[1]);
    }
    if (handleOut < 0){
        fprintf(stderr, "[ERR]: switch X2(%d\n", opCode);
        exit(EXIT_FAILURE); 
        //TODO decidir o que fazer nesta situacao
    }
    if (opCode == 7){
        return 1;
    } return 0;
}



void* threadSender(void* id){
    int* sessionID = (int*) id;
    char buffer[1041];
    char shutdown;
    if (pthread_mutex_lock(&(sessionsMutexes[*sessionID])) < 0){
        //TODO define struct for thread return
    }
    while (1){
        if (!threadBuffers[*sessionID].readable){
            pthread_cond_wait(&sessionsCondVars[*sessionID],&sessionsMutexes[*sessionID]);
        }
        readFromBufferPC(buffer,*sessionID); 
        shutdown = runClientRequest(buffer);
        if (shutdown){
            // sutdown after all has been done
            break;
        }
    } 
    //TODO código se houver shutdown
    if (pthread_mutex_unlock(&(sessionsMutexes[*sessionID])) < 0){
        //TODO define struct for thread return
    }
    exit(0); // isto dá exit ao programa todo

}



int main(int argc, char **argv) {
    pthread_t tid[S+1];
    if (initPthreadVars() < 0){
        return -1;
    }
    initClientsFHandle();
    if (tfs_init()<0){
        return -1;
    }
    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    char *pipename = argv[1];
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    if (pthread_create(&tid[S],0,threadReceiver,pipename) < 0)
        return -1;
    for (int i=0; i<S; i++){
        if (pthread_create(&tid[i],0,threadSender,NULL) < 0)
            return -1; 
    }
    
    return 0;
}


/*int tfs_server_mount(char* joao){

    if(clientCount < S){
        
    }
    return -1;

} TODO check if needed*/ 

/*
switch (opCode)
            {
            case 1:
                handleOut = handle_tfs_mount(&bufferIn[1]);
                break;
            case 2:
                handleOut = handle_tfs_unmount(&bufferIn[1]);
                break;
            case 3:
                handleOut = handle_tfs_open(&bufferIn[1]);
                break;
            case 4:
                handleOut = handle_tfs_close(&bufferIn[1]);
                break;
            case 5:
                handleOut = handle_tfs_write(&bufferIn[1]);
                break;
            case 6:
                handleOut = handle_tfs_read(&bufferIn[1]);
                break;
            case 7:
                handleOut = handle_tfs_shutdown_after_all_closed(&bufferIn[1]);
                break;
            default:
                fprintf(stderr, "[ERR]: switch X1(, %d\n", opCode);
                exit(EXIT_FAILURE);
            }

            if (handleOut < 0){
                fprintf(stderr, "[ERR]: switch X2(%d\n", opCode);
                exit(EXIT_FAILURE); 
            }
        }
*/