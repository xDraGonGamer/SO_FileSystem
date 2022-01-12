#include "operations.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

int tfs_init() {
    state_init();
    pthread_mutex_init(&newFileMutex,NULL);
    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    pthread_mutex_destroy(&newFileMutex);
    state_destroy();
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}


int tfs_lookup(char const *name) {
    // skip the initial '/' character
    name++;
    return find_in_dir(ROOT_DIR_INUM, name);
}

char open_existing_file(int flags, int inum, char* isAppending, size_t* offset){
    /* The file already exists */
    inode_t *inode = inode_get(inum); //gets the inode
    if (inode == NULL) {
        return -1;
    }

    /* Truncate (if requested) */
    if (flags & TFS_O_TRUNC) {
        pthread_rwlock_wrlock(&inode->rwlock);
        if (inode->i_size > 0) {
            if (deleteInodeDataBlocks(inode) == -1) {
                pthread_rwlock_unlock(&inode->rwlock);
                return -1;
            }
            inode->i_size = 0;
            inode->blocksAlloc=0;
        }
        pthread_rwlock_unlock(&inode->rwlock);
    }
    /* Determine initial offset */
    if (flags & TFS_O_APPEND) {
        *offset = inode->i_size;
        *isAppending = 1;
    } else {
        offset = 0;
    }
    return 0;
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset=0;
    char isAppending = 0;

    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }
    inum = tfs_lookup(name); //gets the inumber
    if (inum >= 0) {
        if (open_existing_file(flags,inum,&isAppending,&offset)==-1){
            return -1;
        }
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        pthread_mutex_lock(&newFileMutex);
        inum = tfs_lookup(name);
        if (inum >= 0){
            pthread_mutex_unlock(&newFileMutex);
            if (open_existing_file(flags,inum,&isAppending,&offset)==-1){
                return -1;
            }
            return add_to_open_file_table(inum, offset, isAppending);
        }
        /* Create inode */
        inum = inode_create(T_FILE);
        //ADD: UNLOCK or UNLOCK only after add_dir_entry
        if (inum == -1) {
            pthread_mutex_unlock(&newFileMutex);
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            pthread_mutex_unlock(&newFileMutex);
            return -1;
        }
        pthread_mutex_unlock(&newFileMutex);
        offset = 0;
    } else {
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset, isAppending);

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
    if (inode == NULL) {
        return -1;
    }
    pthread_rwlock_wrlock(&inode->rwlock); //critical zone ahead where writing occurs
    if(file->isAppending){
        file->of_offset = inode->i_size; // if the file truncates after opening for appending
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
        if (allocNecessaryBlocks(inode, sizeNeeded)==-1){
            pthread_rwlock_unlock(&inode->rwlock);
            return -1;
        }

        /* Perform the actual write */
        // memcpy(block + file->of_offset, buffer, to_write);
        size_t blockWriting = divCeilRW(file->of_offset,BLOCK_SIZE); //which block to start writing in
        size_t blockOffset = file->of_offset - ((blockWriting-1) * BLOCK_SIZE); //offset to start writing in block if it has data already
        size_t writingSpace = BLOCK_SIZE-blockOffset; //space left in block to write
        size_t toWriteInBlock = writingSpace < to_write ? writingSpace : to_write;
        size_t bufferOffset = 0;
        char errorHandler;
        void *block;
        while (to_write>0){
            block = getNthDataBlock(inode,blockWriting,&errorHandler);
            if (errorHandler){
                break;
            }
            if (block == NULL) {
                pthread_rwlock_unlock(&inode->rwlock);
                return -1;
            }
            memcpy(block + blockOffset, buffer + bufferOffset, toWriteInBlock);
            bufferOffset+=toWriteInBlock;
            blockOffset = 0;
            to_write-=toWriteInBlock;
            blockWriting++;
            toWriteInBlock = BLOCK_SIZE < to_write ? BLOCK_SIZE : to_write;
        }

        /* The offset associated with the file handle is
         * incremented accordingly */

        file->of_offset += (saveToWrite-to_write);
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }
    pthread_rwlock_unlock(&inode->rwlock);
    return (ssize_t) (saveToWrite-to_write);
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

    if(file->isAppending){ //if reading a file appending, doesnt read
        return 0;
    }
    pthread_rwlock_rdlock(&inode->rwlock); //critical zone ahead where reading occurs
    pthread_mutex_lock(&file->file_entry_mutex);

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }
    
    size_t toReadSave = to_read;

    if (to_read > 0) {
        /* Perform the actual read */
        size_t blockReading = divCeilRW(file->of_offset,BLOCK_SIZE);
        size_t blockOffset = file->of_offset - ((blockReading-1) * BLOCK_SIZE);
        size_t readingSpace = BLOCK_SIZE-blockOffset;
        size_t toReadInBlock = readingSpace < to_read ? readingSpace : to_read;
        size_t bufferOffset = 0;
        char errorHandler;
        void* block;

        while (to_read>0){
            block = getNthDataBlock(inode,blockReading,&errorHandler);
            if (errorHandler){
                pthread_rwlock_unlock(&inode->rwlock);
                pthread_mutex_unlock(&file->file_entry_mutex);
                return -1;
            }
            if (block == NULL) {
                pthread_rwlock_unlock(&inode->rwlock);
                pthread_mutex_unlock(&file->file_entry_mutex);
                return -1;
            }
            memcpy(buffer + bufferOffset, block + blockOffset, toReadInBlock);
            bufferOffset+=toReadInBlock;
            to_read-=toReadInBlock;
            toReadInBlock = BLOCK_SIZE < to_read ? BLOCK_SIZE : to_read;
            blockOffset = 0;
            blockReading++;
        }

        /* The offset associated with the file handle is
         * incremented accordingly */
        file->of_offset += (toReadSave-to_read);
    }
    pthread_rwlock_unlock(&inode->rwlock);
    pthread_mutex_unlock(&file->file_entry_mutex);
    return (ssize_t)(toReadSave-to_read);
}


int tfs_copy_to_external_fs(char const *source_path, char const *dest_path){
    char *buffer = (char*) malloc(BUFFER_SIZE);
    FILE *destFile;
    ssize_t bytesRead;
    /* Checks if the path name is valid */
    if (!valid_pathname(source_path) || tfs_lookup(source_path)==-1) {
        return -1; //source does not exist
    }
    int fhandle = tfs_open(source_path, TFS_O_CREAT);
    if (fhandle<0){
        return -1;
    }
    destFile = fopen(dest_path, "w");
    if (destFile == NULL) {
        tfs_close(fhandle);
        return -1;
    }
    while ((bytesRead=tfs_read(fhandle,buffer,BUFFER_SIZE))>0){
        if (bytesRead<BUFFER_SIZE){
            buffer[bytesRead] = '\0';
        }
        fputs(buffer,destFile);
    }
    if (bytesRead<0){
        return -1;
    }

    fclose(destFile);
    tfs_close(fhandle);
    return 0;
}