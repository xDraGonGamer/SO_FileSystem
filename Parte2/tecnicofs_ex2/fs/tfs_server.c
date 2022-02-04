#include "operations.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define INPUT_BUFFER_SIZE 1041 //buffer for info from client
#define MAX_THREAD_INPUT_SIZE 1041 
#define BUFFER_PARTS 4 //try to implement later


typedef struct {
    char readable;
    char info[MAX_THREAD_INPUT_SIZE];
} bufferPC;


typedef struct {
    int sessionID;
    size_t clientInfoMaxSize;
} clientRequestInfo;

typedef struct {
    int sessionID;
    char* server_pipe_name;
} sender_t;


bufferPC threadBuffers[S]; //consumer-producer buffers for every thread
int clientsFHandle[S]; //client table


pthread_cond_t sessionsCondVars[S];
pthread_mutex_t sessionsMutexes[S];
pthread_mutex_t openClientSessionMutex;

clientRequestInfo nullClientInfo(){
    clientRequestInfo cinfo;
    cinfo.sessionID = 0; cinfo.clientInfoMaxSize = 0;
    return cinfo;
}

char initPCBuffers(){
    for (int i=0;i<S;i++){
        threadBuffers[i].readable = 0;
    }
    return 0;
}

char initPthreadVars(){
    for (int i=0; i<S; i++){
        if ( (pthread_cond_init(&sessionsCondVars[i],NULL) < 0)
        || (pthread_mutex_init(&sessionsMutexes[i],NULL) < 0)){
            return -1;
        }
    }
    if(pthread_mutex_init(&openClientSessionMutex, NULL) < 0){
        return -1;
    }
    return 0;
}

char initClientsFHandle(){
    for (int i=0; i<S; i++){
        clientsFHandle[i] = -1;
    }
    return 0;
}


int getClientFhandle(int sessionId){
    if (sessionId >= S){
        return -1;
    } else {
        return clientsFHandle[sessionId];
    }
}


int openClientSession(char* client_pipe_name, int sessionID){
    int fhandle = open(client_pipe_name,O_WRONLY);
    if (fhandle >= 0){
        if (pthread_mutex_lock(&openClientSessionMutex) < 0){
            fprintf(stderr, "ERROR: Mutex failed to lock.\n");
            exit(EXIT_FAILURE);
        }
        clientsFHandle[sessionID] = fhandle;
        if (pthread_mutex_unlock(&openClientSessionMutex) < 0) {
            fprintf(stderr, "ERROR: Mutex failed to unlock.\n");
            exit(EXIT_FAILURE);
        }
        return fhandle;
    } else {
        fprintf(stderr, "ERROR: open client write channel.\n");
        exit(EXIT_FAILURE);
    }
    
}

int getAvailableSession(){
    if (pthread_mutex_lock(&openClientSessionMutex) < 0){
        fprintf(stderr,"ERROR: Mutex failed to Lock\n");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < S; i++){
        if(clientsFHandle[i] == -1){
            clientsFHandle[i] = -2;
            if (pthread_mutex_unlock(&openClientSessionMutex) < 0){
                fprintf(stderr,"ERROR: Mutex failed to Unlock\n");
                exit(EXIT_FAILURE);
            }
            return i;
        }
    }
    if (pthread_mutex_unlock(&openClientSessionMutex) < 0){
        fprintf(stderr,"ERROR: Mutex failed to Unlock\n");
        exit(EXIT_FAILURE);
    }
    return -1;
}

int finishClientSession(int sessionId){
    if (sessionId >= S){
        return -1;
    }
    if (pthread_mutex_lock(&openClientSessionMutex) < 0){
        fprintf(stderr,"ERROR: Mutex failed to Lock\n");
        exit(EXIT_FAILURE);
    }
    clientsFHandle[sessionId] = -1;
    if (pthread_mutex_unlock(&openClientSessionMutex) < 0){
        fprintf(stderr,"ERROR: Mutex failed to Unlock\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}


void handle_tfs_mount(char *bufferIn){
    char client_pipe_name[40];
    int fclient, sessionID;
    memcpy(&sessionID, bufferIn, sizeof(int));
    strcpy(client_pipe_name,&bufferIn[4]);
    fclient = openClientSession(client_pipe_name, sessionID);
    if (write(fclient,&sessionID,sizeof(int)) < 0 ){
        if (finishClientSession(sessionID) < 0){
            fprintf(stderr,"ERROR: Invalid sessionID detected\n");
        }
        close(fclient);
    } 
}

void handle_tfs_unmount(char *bufferIn){
    int clientSessionID, fclient, out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    out = 0;
    fclient = getClientFhandle(clientSessionID);
    if (fclient<0 || finishClientSession(clientSessionID)<0){
        out = -1;
    }
    if (write(fclient,&out,sizeof(int)) < 0){
        close(fclient);
    } else {
        close(fclient);
    }
    if (finishClientSession(clientSessionID) < 0){
        fprintf(stderr,"ERROR: Invalid sessionID detected\n");
    }
}

void handle_tfs_open(char* bufferIn){
    char fileName[41];
    int clientSessionID, fclient, flags, out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    strcpy(fileName,&bufferIn[4]);
    memcpy(&flags,&bufferIn[4 + strlen(fileName) + 1],sizeof(int));
    out = tfs_open(fileName,flags);

    if (write(fclient,&out,sizeof(int)) < 0){
        if (finishClientSession(clientSessionID) < 0){
            fprintf(stderr,"ERROR: Invalid sessionID detected\n");
        }
        close(fclient);
    } 
}

void handle_tfs_close(char* bufferIn){
    int clientSessionID, fclient, fhandle, out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    memcpy(&fhandle,&bufferIn[4],sizeof(int));
    out = tfs_close(fhandle);

    if (write(fclient,&out,sizeof(int)) < 0){
        if (finishClientSession(clientSessionID) < 0){
            fprintf(stderr,"ERROR: Invalid sessionID detected\n");
        }
        close(fclient);
    } 
}

void handle_tfs_write(char* bufferIn){
    char* toWrite;
    int clientSessionID, fclient, fhandle, out;
    size_t len;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    memcpy(&fhandle,&bufferIn[4],sizeof(int));
    memcpy(&len,&bufferIn[8],sizeof(size_t));
    toWrite = (char*) malloc(sizeof(char)*len); 
    if (toWrite == NULL){
        fprintf(stderr, "ERROR: malloc error\n");
        exit(EXIT_FAILURE);
    }
    memcpy(toWrite,&bufferIn[16],len);
    out = (int) tfs_write(fhandle,toWrite,len);
    if (out != 4){
        fprintf(stderr,"Write failed on source\n");
    }
    free(toWrite);
    if (write(fclient,&out,sizeof(int)) < 0){
        if (finishClientSession(clientSessionID) < 0){
            fprintf(stderr,"ERROR: Invalid sessionID detected\n");
        }
        close(fclient);
    } 
}

void handle_tfs_read(char* bufferIn){
    int clientSessionID, fclient, fhandle, out;
    size_t len;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    memcpy(&fhandle,&bufferIn[sizeof(int)],sizeof(int));
    memcpy(&len,&bufferIn[2*sizeof(int)],sizeof(size_t));
    size_t numberOfBytes = ((sizeof(char)*len)+sizeof(int));
    char bufferOut[numberOfBytes];
    out = (int) tfs_read(fhandle,&bufferOut[sizeof(int)],len);
    memcpy(bufferOut,&out,sizeof(int));
    if (write(fclient,bufferOut,numberOfBytes) < 0){
        if (finishClientSession(clientSessionID) < 0){
            fprintf(stderr,"ERROR: Invalid sessionID detected\n");
        }
        close(fclient);
    }
}


void handle_tfs_shutdown_after_all_closed(char* bufferIn){
    int clientSessionID, fclient, out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    out = tfs_destroy_after_all_closed();

    if (write(fclient,&out,sizeof(int)) < 0){
        if (finishClientSession(clientSessionID) < 0){
            fprintf(stderr,"ERROR: Invalid sessionID detected\n");
        }
        close(fclient);
    } 
}

/* mode 1: Usado quando estamos a ler do pipe do servidor
 mode 0: Usado quando estamos a mandar para o bufferPC*/
char getClientInfoMaxSize(char opCode, size_t* len, char mode){
    switch (opCode) {
    case 1:
        if (mode){
            *len = 40;
        } else {
            *len = 45;
        }
        break;
    case 2:
        *len = (size_t) (5 - mode);
        break;
    case 3:
        *len = (size_t) (49 - mode);
        break;
    case 4:
        *len = (size_t) (9 - mode);
        break;
    case 5:
        *len = (size_t) (1041 - mode);
        break;
    case 6:
        *len = (size_t) (17 - mode);
        break;
    case 7:
        *len = (size_t) (5 - mode);
        break;
    default:
        fprintf(stderr, "ERROR: Client message opCode is wrong (%d)\n",opCode);
        break;
    }
    return 0;
}



clientRequestInfo getClientRequestInfo(char opCode, char* bufferFromClient){
    clientRequestInfo clientInfo = nullClientInfo(); //supress warning
    if (opCode==1){
        clientInfo.sessionID = -1;
    } else {
        memcpy(&(clientInfo.sessionID),&bufferFromClient[1],sizeof(int));
        getClientInfoMaxSize(opCode,&clientInfo.clientInfoMaxSize,0);
    }
    return clientInfo;
}

void sendUserCountFullMessage(char* bufferIn){
    char client_pipe_name[40];
    int out = -1;
    strcpy(client_pipe_name, &bufferIn[1]);
    int fhandle = open(client_pipe_name, O_WRONLY);
    if(write(fhandle, &out, sizeof(int)) < 0){
        fprintf(stderr, "ERROR: Unable to contact client.\n");
        exit(EXIT_FAILURE);
    }
}


void writeToBufferPC(char* bufferIn, clientRequestInfo clientInfo){
    pthread_mutex_lock(&sessionsMutexes[clientInfo.sessionID]);
    memcpy(threadBuffers[clientInfo.sessionID].info,bufferIn,clientInfo.clientInfoMaxSize);
    threadBuffers[clientInfo.sessionID].readable = 1;
    pthread_mutex_unlock(&sessionsMutexes[clientInfo.sessionID]);
}

char readFromPipe(const char* pipename, int pipeFD, void* buffer, size_t nBytes){
    ssize_t readOut = read(pipeFD, buffer, nBytes);
    if (!readOut){
        pipeFD = open(pipename, O_RDONLY);
        if(pipeFD < 0){
            fprintf(stderr, "ERROR: Server failed to open pipe.\n");
            exit(EXIT_FAILURE);
        } else {
            return 1;
        }
    } else if (readOut == -1) { 
        fprintf(stderr, "ERROR: Server failed to read from pipe.\n");
        exit(EXIT_FAILURE);
    } 
    return 0;
}



// código para a thread que recebe info dos clientes
void* threadReceiver(void* server_pipe_name){
    char* pipename = (char*) server_pipe_name;
    size_t bytesToRead = 0;
    clientRequestInfo clientInfo;
    char* bufferIn = (char*) malloc(INPUT_BUFFER_SIZE*sizeof(char));
    if (bufferIn == NULL){
        fprintf(stderr,"ERROR: Malloc error\n");
        exit(EXIT_FAILURE);
    }
    char opCode;
    if(mkfifo(pipename,0777) < 0){
        fprintf(stderr, "ERROR: while creating server channel.\n");
        exit(EXIT_FAILURE);
    }

    int fserver = open(pipename, O_RDONLY);
    if(fserver < 0){
        fprintf(stderr, "ERROR: Server failed to open server channel.\n");
        exit(EXIT_FAILURE);
    }
    while (1){
        if (!readFromPipe(pipename,fserver,&opCode,sizeof(char)) &&
            !getClientInfoMaxSize(opCode,&bytesToRead,1) &&
            !readFromPipe(pipename,fserver,&bufferIn[1],bytesToRead)) { 
            bufferIn[0] = opCode;
            clientInfo = getClientRequestInfo(opCode,bufferIn);
            if (clientInfo.sessionID < 0){
                clientInfo.sessionID = getAvailableSession();
                if(clientInfo.sessionID < 0){
                    sendUserCountFullMessage(bufferIn);
                    continue;
                }else{
                    char newBuffer[45];
                    newBuffer[0] = opCode;
                    memcpy(&newBuffer[1], &clientInfo.sessionID, sizeof(int));
                    strcpy(&newBuffer[5], &bufferIn[1]);
                    clientInfo.clientInfoMaxSize = 45;
                    writeToBufferPC(newBuffer, clientInfo);
                }
            } else {   
                writeToBufferPC(bufferIn, clientInfo);
            }       
            if (pthread_cond_signal(&sessionsCondVars[clientInfo.sessionID]) < 0){
                fprintf(stderr, "ERROR: Failed to signal Conditional Variable.\n");
                exit(EXIT_FAILURE);
            }
            
        }

    }
}

void readFromBufferPC(char* buffer, int sessionID){
    memcpy(buffer,threadBuffers[sessionID].info,1041);
    threadBuffers[sessionID].readable = 0;
}



char runClientRequest(char* info){
    char opCode;
    memcpy(&opCode,info,sizeof(char));
    switch (opCode) {
        case 1:
            handle_tfs_mount(&info[1]);
            break;
        case 2:
            handle_tfs_unmount(&info[1]);
            break;
        case 3:
            handle_tfs_open(&info[1]);
            break;
        case 4:
            handle_tfs_close(&info[1]);
            break;
        case 5:
            handle_tfs_write(&info[1]);
            break;
        case 6:
            handle_tfs_read(&info[1]);
            break;
        default: // case 7
            printf("opCode is %d\n",opCode);
            handle_tfs_shutdown_after_all_closed(&info[1]);
    }
    if (opCode == 7){
        return 1;
    } return 0;
}



void* threadSender(void* arg){
    sender_t* info = (sender_t*) arg;
    int sessionID = info->sessionID;
    char* buffer = (char*) malloc(sizeof(char)*1041);
    char shutdown;
    if (pthread_mutex_lock(&(sessionsMutexes[sessionID])) < 0){
        fprintf(stderr, "ERROR: Internal server fatal error.\n");
        exit(EXIT_FAILURE);
    }
    while (1){
        if (!threadBuffers[sessionID].readable){ //espera para poder ler
            if (pthread_cond_wait(&sessionsCondVars[sessionID],&sessionsMutexes[sessionID]) < 0){
                fprintf(stderr, "ERROR: Conditional Variable failed to wait.\n");
                exit(EXIT_FAILURE);
            }
        }
        readFromBufferPC(buffer,sessionID); 
        shutdown = runClientRequest(buffer);
        if (shutdown){
            // shutdown after all has been done
            break;
        }
    }
    unlink(info->server_pipe_name);
    exit(0);

}



int main(int argc, char **argv) {
    pthread_t tid[S+1];
    if (initPthreadVars() < 0 || initClientsFHandle() < 0 || initPCBuffers() < 0 || tfs_init()<0){
        return -1;
    }
    sender_t senderArgs[S];
    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }
    if(signal(SIGPIPE, SIG_IGN) == SIG_ERR){
        return-1;
    }
    char *pipename = argv[1];
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    if (pthread_create(&tid[S],0,threadReceiver,pipename) < 0){
        unlink(pipename);
        return -1;
    }

    for (int i=0; i<S; i++){
        senderArgs[i].server_pipe_name = pipename;
        senderArgs[i].sessionID = i; 
        if (pthread_create(&tid[i],0,threadSender,&(senderArgs[i])) < 0){
           unlink(pipename);
           return -1; 
        }
    }
    
    for (int i=0 ; i<(S+1) ; i++){
        if (pthread_join(tid[i], NULL) < 0){
            unlink(pipename);
            return -1;
        }
    }    
    
    unlink(pipename);
    return 0;
}