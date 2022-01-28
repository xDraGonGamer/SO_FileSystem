#include "operations.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define INPUT_BUFFER_SIZE 1041 //buffer for info from client

int clientsFHandle[S];


void initClientsFHandle(){
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
    for (int i=0; i<S; i++){
        if (clientsFHandle[i]<0){
            *fhandle = open(client_pipe_name,O_WRONLY);
            if (*fhandle >= 0){
                clientsFHandle[i] = *fhandle;
                return i;
            } else {
                return -1;
            }     
        }
    }
    return -1;
}

int finishClientSession(int sessionId){
    if (sessionId >= S){
        return -1;
    } 
    //TODO maybe lock
    clientsFHandle[sessionId] = -1;
    return 0;

}


ssize_t handle_tfs_mount(char *bufferIn){
    char bufferOut[sizeof(int)];
    char client_pipe_name[40];
    int out, fclient = -1;
    strcpy(client_pipe_name,bufferIn);
    out = openClientSession(client_pipe_name,&fclient);
    if (fclient<0){
        return -1; 
    }
    memcpy(bufferOut,&out,sizeof(int));
    return write(fclient,bufferOut,sizeof(int));
}

ssize_t handle_tfs_unmount(char *bufferIn){
    char bufferOut[sizeof(int)];
    int clientSessionID, fclient, out=-1;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    out = 0;
    fclient = getClientFhandle(clientSessionID);
    if (fclient<=0 || finishClientSession(clientSessionID)<=0 || close(fclient)<=0){
        out = -1;
    }
    // TODO: Falta meter o clientsFHandle[SessionID] a -1, para saber que ha erro
    memcpy(bufferOut,&out,sizeof(int));
    return write(fclient,bufferOut,sizeof(int));
}

ssize_t handle_tfs_open(char* bufferIn){
    char bufferOut[sizeof(int)];
    char fileName[40];
    int clientSessionID, fclient, flags, out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    memcpy(fileName,&bufferIn[4],40);
    memcpy(&flags,&bufferIn[44],sizeof(int));
    out = tfs_open(fileName,flags);

    memcpy(bufferOut,&out,sizeof(int));
    return write(fclient,bufferOut,sizeof(int));
}

ssize_t handle_tfs_close(char* bufferIn){
    char bufferOut[sizeof(int)];
    int clientSessionID, fclient, fhandle, out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    memcpy(&fhandle,&bufferIn[4],sizeof(int));
    out = tfs_close(fhandle);

    memcpy(bufferOut,&out,sizeof(int));
    return write(fclient,bufferOut,sizeof(int));
}

ssize_t handle_tfs_write(char* bufferIn){
    char bufferOut[sizeof(ssize_t)];
    char* toWrite;
    int clientSessionID, fclient, fhandle;
    size_t len;
    ssize_t out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    memcpy(&fhandle,&bufferIn[4],sizeof(int));
    memcpy(&len,&bufferIn[8],sizeof(size_t));
    toWrite = (char*) malloc(sizeof(char)*len); 
    memcpy(toWrite,&bufferIn[16],len);
    out = tfs_write(fhandle,toWrite,len);
    free(toWrite);    
    memcpy(bufferOut,&out,sizeof(ssize_t));
    return write(fclient,bufferOut,sizeof(ssize_t));
}

ssize_t handle_tfs_read(char* bufferIn){
    char bufferOut[(sizeof(ssize_t)+BLOCK_SIZE)];
    int clientSessionID, fclient, fhandle;
    size_t len;
    ssize_t out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    memcpy(&fhandle,&bufferIn[4],sizeof(int));
    memcpy(&len,&bufferIn[8],sizeof(size_t));
    out = tfs_read(fhandle,&bufferOut[sizeof(ssize_t)],len);

    memcpy(bufferOut,&out,sizeof(ssize_t));
    return write(fclient,bufferOut,sizeof(ssize_t)+BLOCK_SIZE);
}


ssize_t handle_tfs_shutdown_after_all_closed(char* bufferIn){
    char bufferOut[sizeof(int)];
    int clientSessionID, fclient, out;
    memcpy(&clientSessionID,bufferIn,sizeof(int));
    fclient = clientsFHandle[clientSessionID];
    out = tfs_destroy_after_all_closed();

    memcpy(bufferOut,&out,sizeof(int));
    return write(fclient,bufferOut,sizeof(int));
}





int main(int argc, char **argv) {
    char bufferIn[INPUT_BUFFER_SIZE];
    ssize_t readOut,handleOut;
    char opCode;
    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    char *pipename = argv[1];
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    if(mkfifo(pipename,0777) < 0){
        exit(1);
    }

    int fserver = open(pipename, O_RDONLY);
    while (1){
        readOut = read(fserver, bufferIn, INPUT_BUFFER_SIZE);
        if (!readOut){ //TODO, just here for debuging (maybe)
            fprintf(stderr, "[INFO]: pipe closed\n");
            return 0;
        } else if (readOut == -1) { //TODO, just here for debuging (maybe)
            fprintf(stderr, "[ERR]: read failed\n");
            exit(EXIT_FAILURE);
        } else {
            memcpy(&opCode,bufferIn,sizeof(char));
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
                fprintf(stderr, "[ERR]: switch X(\n");
                exit(EXIT_FAILURE);
            }

            if (handleOut < 0){
                fprintf(stderr, "[ERR]: switch X(\n");
                exit(EXIT_FAILURE); 
            }
        }


    }

    return 0;
}


/*int tfs_server_mount(char* joao){

    if(clientCount < S){
        
    }
    return -1;

} TODO check if needed*/ 