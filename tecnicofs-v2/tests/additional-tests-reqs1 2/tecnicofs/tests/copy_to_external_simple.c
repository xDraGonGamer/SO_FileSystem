#include "../../../../fs/operations.h"
#include "../../../../fs/operations.c"
#include "../../../../fs/state.h"
#include "../../../../fs/state.c"
#include <assert.h>
#include <string.h>
#include <unistd.h>

int main() {

    char *str = (char*) malloc(2621440);
    memset(str,'a',10249);
    str[10440]='\0';
    char *path = "/f1";
    char *path2 = "external_file.txt";
    char to_read[2621440];

    assert(tfs_init() != -1);

    int file = tfs_open(path, TFS_O_CREAT);
    assert(file != -1);

    assert(tfs_write(file, str, strlen(str)) != -1);

    assert(tfs_close(file) != -1);

    assert(tfs_copy_to_external_fs(path, path2) != -1);

    FILE *fp = fopen(path2, "r");

    assert(fp != NULL);

    assert(fread(to_read, sizeof(char), strlen(str), fp) == strlen(str));
    printf("\n\n\n\n\n\n\n%s-\n\n\n\n\n%s\n",to_read,str);
    assert(strcmp(str, to_read) == 0);

    assert(fclose(fp) != -1);

    unlink(path2);

    printf("Successful test.\n");

    return 0;
}
