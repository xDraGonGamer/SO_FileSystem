#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <time.h>

#define SIZE 1000

int main() {

    char *path = "/f1";
    char input[SIZE];
    memset(input, 'A', SIZE);

    char output [SIZE + 1];

    assert(tfs_init() != -1);

    int f;
    ssize_t r;

    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_write(f, input, SIZE);
    assert(r == SIZE);

    assert(tfs_close(f) != -1);

    f = tfs_open(path, 0);
    assert(f != -1);

    r = tfs_read(f, output, sizeof(output) - 1);
    assert(r == SIZE);

    output[r] = '\0';
    assert(strcmp(output, input) == 0);

    assert(tfs_close(f) != -1);

    printf("Successful test.\n");

    return 0;
}
