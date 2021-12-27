#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int tfs_init() {
    state_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    state_destroy();
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}


int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(ROOT_DIR_INUM, name);
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;

    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }

    inum = tfs_lookup(name); //gets the inumber
    if (inum >= 0) {
        /* The file already exists */
        inode_t *inode = inode_get(inum); //gets the inode
        if (inode == NULL) {
            return -1;
        }

        /* Trucate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                if (deleteInodeDataBlocks(inode) == -1) {
                    return -1;
                }
                inode->i_size = 0;
            }
        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            return -1;
        }
        offset = 0;
    } else {
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset);

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}


int tfs_close(int fhandle) { return remove_from_open_file_table(fhandle); }

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }
    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL || (file->of_offset == inode->i_size) && file->of_offset>0) {
        return -1;
    }

    /* Determine how many bytes to write */
    size_t sizeNeeded = to_write + file->of_offset;
    if (sizeNeeded > MAX_FILE_SIZE) {
        //to_write = MAX_FILE_SIZE - file->of_offset;
        sizeNeeded = MAX_FILE_SIZE;
    }
    to_write = sizeNeeded - file->of_offset;
    size_t saveToWrite = to_write;
    if (to_write > 0) {
        int blocksAlloc;
        if (allocNecessaryBlocks(inode, sizeNeeded)==-1){
            return -1;
        }

        /* Perform the actual write */
        // memcpy(block + file->of_offset, buffer, to_write);
        int blockWriting = !file->of_offset ? 1 : divCeil(file->of_offset,BLOCK_SIZE);
        size_t blockOffset = file->of_offset - ((blockWriting-1) * BLOCK_SIZE);
        int writingSpace = BLOCK_SIZE-blockOffset;
        size_t toWriteInBlock = writingSpace < to_write ? writingSpace : to_write;
        char errorHandler;

        void *block = getNthDataBlock(inode,blockWriting,&errorHandler);
        if (errorHandler){
            return 0;
        }
        if (block == NULL) {
            return -1;
        }
        memcpy(block + blockOffset, buffer, toWriteInBlock);
        to_write-=toWriteInBlock;
        while (to_write>0){
            blockWriting++;
            toWriteInBlock = BLOCK_SIZE < to_write ? BLOCK_SIZE : to_write;
            void *block = getNthDataBlock(inode,blockWriting,&errorHandler);
            if (errorHandler){
                break;
            }
            if (block == NULL) {
                return -1; //FIX when not enough space;
            }
            memcpy(block, buffer, toWriteInBlock);
            to_write-=toWriteInBlock;
        }


        /* The offset associated with the file handle is
         * incremented accordingly */
        file->of_offset += (saveToWrite-to_write);
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }

    return (ssize_t) saveToWrite-to_write;
}


ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }
    size_t toReadSave = to_read;

    if (to_read > 0) {
        /* Perform the actual read */

        int blockReading = divCeil(file->of_offset,BLOCK_SIZE);
        size_t blockOffset = file->of_offset - ((blockReading-1) * BLOCK_SIZE);
        int readingSpace = BLOCK_SIZE-blockOffset;
        size_t toReadInBlock = readingSpace < to_read ? readingSpace : to_read;
        int bufferOffset=0;
        char errorHandler;

        void *block = getNthDataBlock(inode,blockReading,&errorHandler);
        if (errorHandler){
            return 0;
        }
        if (block == NULL) {
            return -1;
        }
        memcpy(buffer, block + blockOffset, toReadInBlock);
        to_read-=toReadInBlock;
        while (to_read>0){
            blockReading++;
            bufferOffset += toReadInBlock;
            toReadInBlock = BLOCK_SIZE < to_read ? BLOCK_SIZE : to_read;
            void *block = getNthDataBlock(inode,blockReading,&errorHandler);
            if (errorHandler){
                //break;
                return -1;
            }
            if (block == NULL) {
                return -1;
            }
            memcpy(buffer, block, toReadInBlock);
            to_read-=toReadInBlock;
        }


        /* The offset associated with the file handle is
         * incremented accordingly */
        file->of_offset += (toReadSave-to_read);
    }

    return (ssize_t)(toReadSave-to_read);
}
