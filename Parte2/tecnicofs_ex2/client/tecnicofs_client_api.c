#include "tecnicofs_client_api.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "string.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


int fclient, fserver, session_id;
char* cpath;

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {
    char buffer[41];
    cpath = (char*) malloc(sizeof(char) * 40);
    buffer[0] = (char) TFS_OP_CODE_MOUNT;
    buffer[1] = '\0';
    strcpy(cpath, client_pipe_path);

    if(mkfifo(client_pipe_path, 0777) < 0){
        return -1;
    }

    if ((fserver = open(server_pipe_path, O_WRONLY)) < 0){
        unlink(client_pipe_path);
        return -1; //abre se o pipe de entrada de comunicacao do servidor
    }
    strcat(buffer, client_pipe_path);
    if (write(fserver, buffer, 41) < 0){
        close(fserver);
        unlink(client_pipe_path);
        return -1;
    }
    if ((fclient = open(client_pipe_path, O_RDONLY)) < 0){
        close(fserver);
        unlink(client_pipe_path);
        return -1; //abre se o pipe de entrada de comunicacao do cliente
    }
    if (read(fclient, &session_id, sizeof(int)) < 0){
        close(fclient);
        close(fserver);
        unlink(client_pipe_path);
        return -1;
    }
    if(session_id == -1){
        close(fclient);
        close(fserver);
        unlink(client_pipe_path);
        return -1;
    }
    return 0;
}

int tfs_unmount() {
    char buffer[5];
    int serverResponse;
    buffer[0] = TFS_OP_CODE_UNMOUNT;
    memcpy(&buffer[1], &session_id, sizeof(int));
    if (write(fserver, buffer, 41) < 0)
        return -1;
    if (read(fclient, &serverResponse, sizeof(int)) < 0)
        return -1;
    if (serverResponse>=0){
        close(fclient);
        close(fserver);
        unlink(cpath);
    }
    return serverResponse;
}

int tfs_open(char const *name, int flags) {
    char buffer[49];
    int serverResponse;
    buffer[0] = TFS_OP_CODE_OPEN;
    memcpy(&buffer[1], &session_id, sizeof(int));
    memcpy(&buffer[1 + sizeof(int)], name, strlen(name) + 1);
    memcpy(&buffer[1 + sizeof(int) + strlen(name) + 1], &flags, sizeof(int));
    if (write(fserver, buffer, 49) < 0)
        return -1;
    if (read(fclient, &serverResponse, sizeof(int)) < 0)
        return -1;
    return serverResponse;
}

int tfs_close(int fhandle) {
    char buffer[9];
    int serverResponse;
    buffer[0] = TFS_OP_CODE_CLOSE;
    memcpy(&buffer[1], &session_id, sizeof(int));
    memcpy(&buffer[1 + sizeof(int)], &fhandle, sizeof(int));
    if (write(fserver, buffer, 9) < 0)
        return -1;
    if (read(fclient, &serverResponse, sizeof(int)) < 0)
        return -1;
    return serverResponse;
}

int tfs_write(int fhandle, void const *buffer, size_t len) {
    char wBuffer[1041];
    int serverResponse;
    wBuffer[0] = TFS_OP_CODE_WRITE;
    memcpy(&wBuffer[1], &session_id, sizeof(int));
    memcpy(&wBuffer[1+sizeof(int)], &fhandle, sizeof(int));
    memcpy(&wBuffer[1+2*(sizeof(int))], &len, sizeof(size_t));
    memcpy(&wBuffer[1+2*(sizeof(int))+sizeof(size_t)], buffer , len);
    if (write(fserver, wBuffer, 1041) < 0)
        return -1;
    if (read(fclient, &serverResponse, sizeof(int)) < 0)
        return -1;
    return serverResponse;
}

int tfs_read(int fhandle, void *buffer, size_t len) {
    char* myBuffer = (char*) buffer;
    size_t readBytesFromServer = len+sizeof(ssize_t);
    char rBuffer[readBytesFromServer]; // read info from server
    char wBuffer[17]; // write info to server  
    int serverResponse;
    wBuffer[0] = TFS_OP_CODE_READ;
    memcpy(&wBuffer[1], &session_id, sizeof(int));
    memcpy(&wBuffer[1+sizeof(int)], &fhandle, sizeof(int));
    memcpy(&wBuffer[1+2*(sizeof(int))], &len, sizeof(size_t));
    if (write(fserver, wBuffer, 17) < 0)
        return -1;
    if (read(fclient, &rBuffer, readBytesFromServer) < 0)
        return -1;
    memcpy(&serverResponse,rBuffer,sizeof(int));
    if (serverResponse>0){
        memcpy(myBuffer,&rBuffer[sizeof(ssize_t)], (size_t) serverResponse);
    } 
    return serverResponse;
}

int tfs_shutdown_after_all_closed() {
    char buffer[5];
    int serverResponse;
    buffer[0] = TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED;
    buffer[1] = '\0';
    strncat(buffer, (void*)&session_id, sizeof(int));
    if (write(fserver, buffer, 5) < 0)
        return -1;
    if (read(fclient, &session_id, sizeof(int)) < 0)
        return -1;
    return serverResponse;
}