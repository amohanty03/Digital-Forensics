#include "ext2_fs.h"
#include "read_ext2.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>


int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("expected usage: ./runscan inputfile outputfile\n");
        exit(0);
    }

    // Opening output directory
    char *outputDir = argv[2];
    int outputDirStatus = mkdir(outputDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (outputDirStatus == -1) {
        perror("Failed to create directory.\n");
        exit(1);
    } 

    int fd;
    fd = open(argv[1], O_RDONLY);    /* open disk image */

    ext2_read_init(fd);

    struct ext2_super_block super;
    struct ext2_group_desc group;

    // example read first the super-block and group-descriptor
    read_super_block(fd, 0, &super);
    read_group_desc(fd, 0, &group);

    // Actual code for runscan.c
    


    return 0;
}