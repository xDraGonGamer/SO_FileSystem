#include "operations.h"
#include <fcntl.h>

#define INPUT_BUFFER_SIZE 1041 //buffer for info from client

int clientCount = 0;

int main(int argc, char **argv) {
    char buffer[INPUT_BUFFER_SIZE];
    int clientCount = 0;
    ssize_t ret;
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
        ret = read(fserver, buffer, INPUT_BUFFER_SIZE);
        if (!ret){ //TODO, just here for debuging (maybe)
            fprintf(stderr, "[INFO]: pipe closed\n");
            return 0;
        } else if (ret == -1) { //TODO, just here for debuging (maybe)
            fprintf(stderr, "[ERR]: read failed\n");
            exit(EXIT_FAILURE);
        } else {
            memcpy(&opCode,buffer,sizeof(char));
            switch (opCode)
            {
            case 1:
                /* code */
                break;
            case 2:
                /* code */
                break;
            case 3:
                /* code */
                break;
            case 4:
                /* code */
                break;
            case 5:
                /* code */
                break;
            case 6:
                /* code */
                break;
            case 7:
                /* code */
                break;
            default:
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