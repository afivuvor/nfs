#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define block_size 1024

typedef struct superblock {
	char datablocks[block_size*100];		//total number of data blocks
	char data_bitmap[105];      			//array of data block numbers that are available
	char inode_bitmap[105];   				//array of inode numbers that are available
} superblock;

typedef struct inode {
	int datablocks[16];            //data block number that the inode points to
	int number;
	int blocks;                    //==number of blocks the particular inode points to
	int size;                      //==size of file/directory
} inode;

typedef struct filetype {
	int valid;
	char test[10];
	char path[100];
	char name[100];           //name
	inode *inum;              //inode number
	struct filetype ** children;
	int num_children;
	int num_links;
	struct filetype * parent;
	char type[20];                  //==file extension
	mode_t permissions;		        // Permissions
	uid_t user_id;		            // userid
	gid_t group_id;		            // groupid
	time_t a_time;                  // Access time
	time_t m_time;                  // Modified time
	time_t c_time;                  // Status change time
	time_t b_time;                  // Creation time
	off_t size;                     // Size of the node

	int datablocks[16];
	int number;
	int blocks;

} filetype;

filetype * root;
filetype file_array[200];
superblock spblock;

void initialize_superblock();
void tree_to_array(filetype * queue, int * front, int * rear, int * index);
int save_contents();
void initialize_root_directory();
filetype * filetype_from_path(char * path);
int find_free_inode();
int find_free_db();
void add_child(filetype * parent, filetype * child);
static int fsgetattr(const char *path, struct stat *statit);
static int fsmkdir(const char *path, mode_t mode);
int fsreaddir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi );
int fscreate(const char * path, mode_t mode, struct fuse_file_info *fi);
int fsopen(const char *path, struct fuse_file_info *fi);
int fsread(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int fswrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int fsrelease(const char *path, struct fuse_file_info *fi);

#endif // FILESYSTEM_H