#include "operations.h"

int clientCount = 0;

int main(int argc, char **argv) {

    int clientCount = 0;

    if (argc < 2) {
        printf("Please specify the pathname of the server's pipe.\n");
        return 1;
    }

    char *pipename = argv[1];
    printf("Starting TecnicoFS server with pipe called %s\n", pipename);

    if(mkfifo(pipename) < 0){
        exit(1);
    }

    /* TO DO */

    return 0;
}


int tfs_server_mount(char* joao){

    if(clientCount < S){
        
    }
    return -1;

}