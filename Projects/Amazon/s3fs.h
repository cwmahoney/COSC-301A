#ifndef __USERSPACEFS_H__
#define __USERSPACEFS_H__

#include <sys/stat.h>
#include <stdint.h>   // for uint32_t, etc.
#include <sys/time.h> // for struct timeval

/* This code is based on the fine code written by Joseph Pfeiffer for his
   fuse system tutorial. */

/* Declare to the FUSE API which version we're willing to speak */
#define FUSE_USE_VERSION 26

#define S3ACCESSKEY "S3_ACCESS_KEY_ID"
#define S3SECRETKEY "S3_SECRET_ACCESS_KEY"
#define S3BUCKET "S3_BUCKET"

#define BUFFERSIZE 1024

// store filesystem state information in this struct
typedef struct {
    char s3bucket[BUFFERSIZE];
} s3context_t;

/*
 * Other data type definitions (e.g., a directory entry
 * type) should go here.
 */

typedef struct {
	unsigned char type; // file f, directory d, or unused u
	char name[256]; 	// reasonable upper-bound on a name
	
	// metadata goes here
	struct stat *metadata;

	/*
	struct stat {
    dev_t     st_dev;     //ID of device containing file
    ino_t     st_ino;     //inode number
    mode_t    st_mode;    //protection
    nlink_t   st_nlink;   //number of hard links
    uid_t     st_uid;     //user ID of owner
    gid_t     st_gid;     //group ID of owner
    dev_t     st_rdev;    //device ID (if special file)
    off_t     st_size;    //total size, in bytes
    blksize_t st_blksize; // blocksize for file system I/O
    blkcnt_t  st_blocks;  //number of 512B blocks allocated
    time_t    st_atime;   //time of last access
    time_t    st_mtime;   //time of last modification
    time_t    st_ctime;   //time of last status change
	*/
//Sommers: Yes for the timestamps and gid (just use the getgid() call to get the group id for the current user; same thing for uid --- just use getuid()).  No for st_blocks, st_rdev.  st_nlink should just be 1.
	/* Only using:
	
    mode_t    st_mode;    //protection
    uid_t     st_uid;     //user ID of owner
    time_t    st_atime;   //time of last access
    time_t    st_mtime;   //time of last modification

	nlink_t   st_nlink;   //number of hard links - should be 1
	gid_t     st_gid;     //group ID of owner
	time_t    st_ctime;   //time of last status change - Read: Create time
	*/
} s3dirent_t;

#endif // __USERSPACEFS_H__
