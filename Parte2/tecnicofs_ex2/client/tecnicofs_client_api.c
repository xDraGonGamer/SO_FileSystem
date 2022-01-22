#include "tecnicofs_client_api.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "string.h"
#include <unistd.h>

int fclient, fserver, session_id;
char* cpath, spath;

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {
    char buffer[41];
    buffer[0] = '0' + TFS_OP_CODE_MOUNT;
    buffer[1] = '\0';
    strcopy(cpath, client_pipe_path);
    strcopy(spath, server_pipe_path);
    if ((fserver = open(server_pipe_path, O_WRONLY)) < 0) return -1; //abre se o pipe de entrada de comunicacao do servidor
    if ((fclient = open(client_pipe_path, O_RDONLY)) < 0) return -1; //abre se o pipe de entrada de comunicacao do cliente
    strcat(buffer, client_pipe_path);
    write(fserver, buffer, 41);
    read(fclient, &session_id, sizeof(int));
    if(session_id == -1){
        close(fclient);
        close(fserver);
        return -1;
    }
    return 0;
}

int tfs_unmount() {
    if ((fserver = open(spath, O_WRONLY)) < 0) return -1;
    if ((fclient = open(cpath, O_RDONLY)) < 0) return -1;
    char buffer[5];
    buffer[0] = '0' + TFS_OP_CODE_MOUNT;
    buffer[1] = '\0';
    strncat(buffer, (void*)&session_id, sizeof(int));
    write(fserver, buffer, 41);
    close(fclient);
    close(fserver);
    return -1;
}

int tfs_open(char const *name, int flags) {
    if ((fserver = open(spath, O_WRONLY)) < 0) return -1;
    if ((fclient = open(cpath, O_RDONLY)) < 0) return -1;
    char buffer[49];
    buffer[0] = '0' + TFS_OP_CODE_OPEN;
    buffer[1] = '\0';
    strncat(buffer, (void*)&session_id, sizeof(int));
    strcat(buffer, name);
    strncat(buffer, (void*)&flags, sizeof(int));
    write(fserver, buffer, 49);
    close(fclient);
    close(fserver);
    return -1;
}

int tfs_close(int fhandle) {
    if ((fserver = open(spath, O_WRONLY)) < 0) return -1;
    if ((fclient = open(cpath, O_RDONLY)) < 0) return -1;
    char buffer[9];
    buffer[0] = '0' + TFS_OP_CODE_CLOSE;
    buffer[1] = '\0';
    strncat(buffer, (void*)&session_id, sizeof(int));
    strncat(buffer, (void*)&fhandle, sizeof(int));
    write(fserver, buffer, 9);
    close(fclient);
    close(fserver);
    return -1;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
    /* TODO: Implement this */
    return -1;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    /* TODO: Implement this */
    return -1;
}

int tfs_shutdown_after_all_closed() {
    if ((fserver = open(spath, O_WRONLY)) < 0) return -1;
    if ((fclient = open(cpath, O_RDONLY)) < 0) return -1;
    char buffer[5];
    buffer[0] = '0' + TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED;
    buffer[1] = '\0';
    strncat(buffer, (void*)&session_id, sizeof(int));
    write(fserver, buffer, 5);
    close(fclient);
    close(fserver);
    return -1;
}
