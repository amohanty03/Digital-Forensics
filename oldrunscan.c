// Copyright 2022 <Copyright Nikhil>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "./ext2_fs.h"
#include "./read_ext2.h"

// Global data
// typedefs from ext2_fs.h
typedef __uint16_t      __u16;
typedef __uint32_t      __u32;
typedef unsigned char   __u8;
int direct_block_count = 12;
int single_indirect_block_count = 1;
int double_indirect_block_count = 1;
int pointer_size = 4;
int pointers_per_block = 256;

int round_up(int num_to_round, int multiple) {
    if (multiple == 0)
        return num_to_round;

    int remainder = num_to_round % multiple;
    if (remainder == 0)
        return num_to_round;

    return num_to_round + multiple - remainder;
}

void read_data_block(int req_block_size, int inod_num,  int fd,
 int curr_block, struct ext2_inode *inode, int output) {
    char temp_buffer[block_size];
    lseek(fd, BLOCK_OFFSET(inode->i_block[curr_block]), SEEK_SET);
    read(fd, temp_buffer, req_block_size);

    int write_out = write(output, temp_buffer, req_block_size);
    if (write_out == -1) {
        printf("writing to inode %u failed\n", inod_num);
        exit(1);
    }
}

int main(int argc, char **argv) {
        if (argc != 3) {
                printf("expected usage: ./runscan inputfile outputfile\n");
                exit(0);
        }
        __u32 max_single_indirect_bytes =
        (direct_block_count * block_size) +
        (single_indirect_block_count * pointers_per_block * block_size);

         __u32 max_double_indirect_bytes = max_single_indirect_bytes +
        (double_indirect_block_count * pointers_per_block *
         pointers_per_block * block_size);

        // 798,720 and 67,907,584 respectively

//  might be able to use!
//        __u32 max_regular_bytes = direct_block_count * block_size; // 12*1024

        // create an output directory from argv[2] name
        char *output_dir = argv[2];
        int dir_ret = mkdir(output_dir, S_IRWXU);
        if (dir_ret == -1) {
            switch (errno) {
                case EEXIST:
                    printf("directory name already exists!\n");
                    exit(1);
                default:
                    printf("mkdir error");
                    exit(1);
            }
        }

        // given source code
        int fd;

        fd = open(argv[1], O_RDONLY);    /* open disk image */

// the disk image is opened and all common variables are initialized in
// the read_ext2.c file that is compiled along with runscan.c
// see read_ext2.c for all the variables that are initialized here!

        ext2_read_init(fd);

// Read the super block and the group descriptor into these structs
// these structs below and their fields are defined in ext2_fs.h
// which is included along with read_ext2.c and
// the methods it provides!

        struct ext2_super_block super;
        struct ext2_group_desc group;

        // example read first the super-block and group-descriptor
        read_super_block(fd, 0, &super);
        read_group_desc(fd, 0, &group);

// Outputs the inodes in one block of inode table,
// and the number of block the inode table occupies

        // printf("There are %u inodes in an inode table block and
        // %u blocks in the inode table\n", inodes_per_block, itable_blocks);

// Since the group_descriptor blocks give us the location of the inode table
// in the block group, it is used to find the start
// address of the inode table and it stored in start_inode_table
// (as off_t, a signed integer type!)
// this start address will be used to iterate over the inodes in the
// inode table, where the fun begins!

        // iterate the first inode block
        off_t start_inode_table = locate_inode_table(0, &group);


// write code for real runscan.c here
    unsigned int total_inodes_count = inodes_per_block * itable_blocks;

// Iterate over total inodes in the disk image,
// or maximum inodes iterated over marked here
// might have to change max nodes possible to iterate over(total_inodes_count
// to just the first non-reserved inode!

// Hypothetically iterate over all inodes in disk image
// Will iterate only if directory or file is subsequent inode, else breaks

// loop counter i is initialized to first non-reserved inode
// incremented in loop iteration!
// printf("\n\ntotal_inodes_count: %u\n\n", total_inodes_count);
    for (unsigned int i = 2; i < total_inodes_count; i++) {
        struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));

        // read the current (possibly) non-reserved inode into inode
        read_inode(fd, 0, start_inode_table, i, inode);

        if (S_ISDIR(inode->i_mode)) {
            // implement now

            // printf("\n\nEntering a directory  || Inode number: %d\n\n", i);

            // read entries of the directory
            struct ext2_dir_entry_2 *entry;
            unsigned int size;  // keep track of bytes read
            unsigned char block[block_size];

            lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
            read(fd, block, block_size);

            size = 0;  // keep track of bytes
            entry = (struct ext2_dir_entry_2 *) block;  // first dir entry
            while (size < inode->i_size) {
                char file_name[EXT2_NAME_LEN+1];
                memcpy(file_name, entry->name, entry->name_len);
                file_name[entry->name_len] = 0;
               // printf("%10u %s\n", entry->inode, file_name);


                // If the file is a regular file and a jpg file,
                // then open and create a new file with the same
                // name stored in filename, and write all bytes to it
                // similar to how it's done with the inode name files
                if (entry->file_type == (__u32)1) {
                   struct ext2_inode *inner_inode =
                   malloc(sizeof(struct ext2_inode));
                   read_inode(fd, 0, start_inode_table,
                   (int)entry->inode, inner_inode);

                   char store_buffer[block_size];
                   lseek(fd, BLOCK_OFFSET(inner_inode->i_block[0]), SEEK_SET);
                   read(fd, store_buffer, block_size);


                   int is_jpg = 0;
                   if (store_buffer[0] == (char)0xff &&
                       store_buffer[1] == (char)0xd8 &&
                       store_buffer[2] == (char)0xff &&
                      (store_buffer[3] == (char)0xe0 ||
                       store_buffer[3] == (char)0xe1 ||
                       store_buffer[3] == (char)0xe8)) {
                       is_jpg = 1;
                   }

                   if (is_jpg) {
                       char filename[1024];
                       strncpy(filename, argv[2], sizeof(filename));

                       char slash_file_name[1024] = "/";
                       strcat(slash_file_name, file_name);
                       strcat(filename, slash_file_name);

                       // write code for opening a file,
                       // same as file code below
                       int open_output = open(filename,
                       O_CREAT|O_WRONLY|O_TRUNC, 0777);
                       if (open_output == -1) {
                           printf("call to open failed for regular file");
                           exit(1);
                       }


                       __u32 inner_inode_bytes_left = inner_inode->i_size;

                       int inner_single_flag = 0;
                       int inner_double_flag = 0;
                       int inner_remainder_flag = 0;

                       if (inner_inode_bytes_left > direct_block_count *
                           block_size && inner_inode_bytes_left
                           <= max_single_indirect_bytes) {
                           inner_single_flag = 1;
                       }

                       if (inner_inode_bytes_left > max_single_indirect_bytes
                           && inner_inode_bytes_left
                           <= max_double_indirect_bytes) {
                           inner_single_flag = 1;
                           inner_double_flag = 1;
                       }

                       int inner_curr_inode_blocks;
                       int inner_final_block_remainder;

                       if (inner_single_flag) {
                           inner_curr_inode_blocks = 12;
                           inner_final_block_remainder = 0;
                       } else {
                           inner_curr_inode_blocks =
                           (int)(inner_inode_bytes_left / 1024);
                           inner_final_block_remainder =
                           (int)(inner_inode_bytes_left % 1024);
                       }

                       if (inner_inode_bytes_left <= direct_block_count
                           * block_size && inner_final_block_remainder != 0) {
                           inner_remainder_flag = 1;
                       }

                       int j2 = 0;
                       for (j2 = 0; j2 < inner_curr_inode_blocks; j2++) {
                           read_data_block(block_size,
                           (int)entry->inode,
                           fd, j2, inner_inode, open_output);
                       }

                       if (inner_remainder_flag) {
                           read_data_block(inner_final_block_remainder,
                           (int)entry->inode,
                           fd, j2, inner_inode, open_output);
                       }

                       if (inner_single_flag) {
                           unsigned int single_ind_array[pointers_per_block];

                           lseek(fd, BLOCK_OFFSET(inner_inode->i_block[12]),
                           SEEK_SET);
                           read(fd, single_ind_array, block_size);

                           for (int g = 0; g < pointers_per_block; g++) {
                               char data_buffer_g[block_size];
                               lseek(fd, BLOCK_OFFSET(single_ind_array[g]),
                               SEEK_SET);
                               read(fd, data_buffer_g, block_size);

                               int write_output_2 = write(open_output,
                               data_buffer_g, block_size);
                               if (write_output_2 == -1) {
                                   printf("writing failed in single indirect");
                                   exit(1);
                               }
                           }
                       }

                       if (inner_double_flag) {
                           int inner_double_bytes = inner_inode->i_size -
                           (12 * block_size) -
                           (pointers_per_block * block_size);
                           int inner_double_blocks =
                           inner_double_bytes / block_size;
                           int inner_byte_remainder =
                           inner_inode->i_size % block_size;

                           unsigned int
                           inner_pointer_array[pointers_per_block];
                           lseek(fd, BLOCK_OFFSET(inner_inode->i_block[13]),
                           SEEK_SET);
                           read(fd, inner_pointer_array, block_size);

                           int inner_break_flag = 0;
                           for (int x = 0; x < pointers_per_block; x++) {
                               unsigned int
                               inner_inner_pointer_array[pointers_per_block];
                               lseek(fd, BLOCK_OFFSET(inner_pointer_array[x]),
                               SEEK_SET);
                               read(fd, inner_inner_pointer_array, block_size);

                               // inner loop starts here
                               for (int y = 0; y < pointers_per_block; y++) {
                                   if (inner_double_blocks == 0) {
                                    char
                                    inner_double_buffer[inner_byte_remainder];
                                    lseek(fd,
                                    BLOCK_OFFSET(inner_inner_pointer_array[y]),
                                    SEEK_SET);
                                    read(fd, inner_double_buffer,
                                    inner_byte_remainder);

                                       int inner_write_out =
                                       write(open_output, inner_double_buffer,
                                       inner_byte_remainder);
                                       if (inner_write_out == -1) {
                                           printf("double indirect failed");
                                           exit(1);
                                       }
                                   } else {
                                   char inner_double_buffer[block_size];
                                   lseek(fd,
                                   BLOCK_OFFSET(inner_inner_pointer_array[y]),
                                   SEEK_SET);
                                   read(fd, inner_double_buffer, block_size);

                                       int inner_write_out =
                                       write(open_output,
                                       inner_double_buffer, block_size);
                                       if (inner_write_out == -1) {
                                           printf("double indirect failed");
                                           exit(1);
                                       }
                                   }
                                   inner_double_blocks--;
                                   if (inner_double_blocks == -1) {
                                       inner_break_flag = 1; break;
                                   }
                               }
                               if (inner_break_flag) { break; }
                           }
                       }
                       close(open_output);
                       free(inner_inode);
                }
        }
                // define variable offset that checks against rec_len
                // 4(inode) + 2(rec_len) + 1(name_len) + 1(file_type) = 8
                // 8 minimum + name_len rounded up to a multiple of 4

                int real_offset = 8;  // since 8 is minimum
                int name_len_rounded = round_up(entry->name_len, 4);
                real_offset += name_len_rounded;

                // real_offset may or may not be the same as entry->rec_len
                // since this does not show deleted inodes

                entry = (void*) entry + real_offset;  // replace rec_len
                size += real_offset;
            }

            free(inode);
            continue;
        } else if (S_ISREG(inode->i_mode)) {
          // printf("\n\nEntering a regular file  || Inode number: %d\n\n", i);

            // maximum index of i_block array for current inode

           // printf("inode->i_blocks is %d\n", inode->i_blocks);

            // the inode is a regular file, check if it is a jpg
            // first, read first data block of inode

            char buffer[block_size];
            lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
            read(fd, buffer, block_size);

        //    printf("\n");
        //    printf("inode number --> %d\n", i);

            int is_jpg = 0;
            if (buffer[0] == (char)0xff &&
                buffer[1] == (char)0xd8 &&
                buffer[2] == (char)0xff &&
               (buffer[3] == (char)0xe0 ||
                buffer[3] == (char)0xe1 ||
                buffer[3] == (char)0xe8)) {
                is_jpg = 1;
            }

            if (!is_jpg) { continue; }  // exit loop if not jpg file

            // Now, we have a jpg file
            // put the jpg file in output directory
            char main_filename[1024] = "/file-";
            char inode_num_char[10];
            snprintf(inode_num_char, sizeof(inode_num_char), "%d", i);

            char temp_jpg[5] = ".jpg";
            strcat(main_filename, inode_num_char);
            strcat(main_filename, temp_jpg);
            // printf("main_filename --> %s\n", main_filename);
            // printf("inode_num_char --> %s\n", inode_num_char);

            char filename[1024];
            strncpy(filename, argv[2], sizeof(filename));
            strcat(filename, main_filename);
         //   printf("filename --> %s\n", filename);

            int output = open(filename, O_CREAT|O_WRONLY|O_TRUNC, 0777);
            if (output == -1) {
                printf("call to open failed for regular file %s\n", filename);
                exit(1);
            }

            __u32 inode_bytes_left = inode->i_size;
          //  printf("inode_bytes_left --> %u\n", inode_bytes_left);

            int single_flag = 0;
            int double_flag = 0;
            int remainder_flag = 0;

            if (inode_bytes_left > direct_block_count *
                block_size && inode_bytes_left
                <= max_single_indirect_bytes) {
                single_flag = 1;
            }

            if (inode_bytes_left > max_single_indirect_bytes &&
                inode_bytes_left <= max_double_indirect_bytes) {
                single_flag = 1;
                double_flag = 1;
            }

             int curr_inode_blocks;
             int final_block_remainder;

             if (single_flag) {
                 curr_inode_blocks = 12;
                 final_block_remainder = 0;
             } else {  // single flag does not exist
                 curr_inode_blocks = (int)(inode_bytes_left / 1024);
                 final_block_remainder = (int) (inode_bytes_left % 1024);
             }

           //  printf("curr_inode_blocks --> %d\n", curr_inode_blocks);
           //  printf("final_block_remainder --> %d\n", final_block_remainder);

             if (inode_bytes_left <= direct_block_count *
                 block_size && final_block_remainder != 0) {
                 remainder_flag = 1;
             }


            // first iterate over direct data blocks (12 max)
            // printf("entered direct data block iteration\n");

                int j = 0;
                for (j = 0; j < curr_inode_blocks; j++) {
              //      printf("1\n");
                    read_data_block(block_size, i, fd, j, inode, output);
                }

                // add remainder block data if non-whole data block exists
                if (remainder_flag) {
                //    printf("2\n");
                    read_data_block(final_block_remainder,
                    i, fd, j, inode, output);
                }

            if (single_flag) {
                // make an unsigned integer array of pointers
                // all these pointers point to data_blocks which we need to
                // read into our file
                unsigned int sing_ind_array[pointers_per_block];

                lseek(fd, BLOCK_OFFSET(inode->i_block[12]), SEEK_SET);
                read(fd, sing_ind_array, block_size);

                // Iterate over all pointers in pointer array
                for (int k = 0; k < pointers_per_block; k++) {
                    // create a buffer that stores a data blocks information
                    // does this for every pointer
                    char data_buffer[block_size];
                    lseek(fd, BLOCK_OFFSET(sing_ind_array[k]), SEEK_SET);
                    read(fd, data_buffer, block_size);

                    int write_out = write(output, data_buffer, block_size);
                    if (write_out == -1) {
                        printf("writing failed in single indirect block\n");
                        exit(1);
                    }
                }
            }

            if (double_flag) {
            // bytes calculation
            int double_bytes = inode->i_size - (12 * block_size) -
            (pointers_per_block * block_size);
            int double_blocks = double_bytes / block_size;
            int byte_remainder = inode->i_size % block_size;

                // refer to previous code and pseudocode
                unsigned int pointer_array[pointers_per_block];
                lseek(fd, BLOCK_OFFSET(inode->i_block[13]), SEEK_SET);
                read(fd, pointer_array, block_size);

                int break_flag = 0;
                for (int n = 0; n < pointers_per_block; n++) {
                    unsigned int inner_pointer_array[pointers_per_block];
                    lseek(fd, BLOCK_OFFSET(pointer_array[n]), SEEK_SET);
                    read(fd, inner_pointer_array, block_size);

                    // inner loop starts here
                    for (int m = 0; m < pointers_per_block; m++) {
                        if (double_blocks == 0) {
                        char double_buffer[byte_remainder];
                        lseek(fd, BLOCK_OFFSET(inner_pointer_array[m]),
                        SEEK_SET);
                        read(fd, double_buffer, byte_remainder);

                        int write_out =
                        write(output, double_buffer, byte_remainder);
                        if (write_out == -1) {
                            printf("double indirect failed\n");
                            exit(1);
                        }
                        } else {
                        char double_buffer[block_size];
                        lseek(fd,
                        BLOCK_OFFSET(inner_pointer_array[m]), SEEK_SET);
                        read(fd, double_buffer, block_size);

                        int write_out = write(output,
                        double_buffer, block_size);
                        if (write_out == -1) {
                            printf("writing failed in double indirect block\n");
                            exit(1);
                        }
                        }
                        double_blocks--;
                        if (double_blocks == -1) {
                            break_flag = 1; break;
                        }
                    }
                    if (break_flag) { break; }
                }
            }
            // close the opened file
            close(output);
            free(inode);
        } else {
            // printf("\n\nEntering neither a file nor a directory
            // || Inode number: %d\n\n", i);
            free(inode);
            continue;  // continue iterating over inodes
        }
    }

// Comment marker 2
        close(fd);
}





/////////////////


 if (z == 0) {
                                prev = inode->i_block[z];
                                lseek(fd, BLOCK_OFFSET(inode->i_block[z]), SEEK_SET);
                                read(fd, buffer, block_size);
                                fwrite(buffer, block_size, 1, fp);
                            } else {
                                
                                if (abs(prev - inode->i_block[z]) > 1) {
                                    unsigned int blockNums[256]; 
                                    memcpy(blockNums,&inode->i_block[z], 256);
                                    for (int k =0; k < 256; k++) {
                                        printf("copying %d\n", k);
                                        if (blockNums[k] != 0) {
                                            lseek(fd, BLOCK_OFFSET(blockNums[k]), SEEK_SET);
                                            read(fd, buffer, block_size);
                                            fwrite(buffer, block_size, 1, fp);
                                        }
                                    }
                                } else {
                                    
                                    prev = inode->i_block[z];
                                }
                                
                            }