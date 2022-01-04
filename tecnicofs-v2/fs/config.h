#ifndef CONFIG_H
#define CONFIG_H

/* FS root inode number */
#define ROOT_DIR_INUM (0)

#define BLOCK_SIZE (1024)
#define DATA_BLOCKS (1024)
#define INODE_TABLE_SIZE (50)
#define MAX_OPEN_FILES (20)
#define MAX_FILE_NAME (40)
#define DIRECT_BLOCK_COUNT (10)
#define MAX_FILE_SIZE (2621440) //might be global
#define MAX_INODE_BLOCKS (266) //might be global
#define BUFFER_SIZE (256)

#define DELAY (5000)

#endif // CONFIG_H
