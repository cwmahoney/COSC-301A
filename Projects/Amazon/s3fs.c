/* This code is based on the fine code written by Joseph Pfeiffer for his
   fuse system tutorial. */

/* When adding directories, they should have a / at the end!! */

#include "s3fs.h"
#include "libs3_wrapper.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

#define GET_PRIVATE_DATA ((s3context_t *) fuse_get_context()->private_data)

/*
 * For each function below, if you need to return an error,
 * read the appropriate man page for the call and see what
 * error codes make sense for the type of failure you want
 * to convey.  For example, many of the calls below return
 * -EIO (an I/O error), since there are no S3 calls yet
 * implemented.  (Note that you need to return the negative
 * value for an error code.)
 */

void growDir(s3dirent_t *entry, s3dirent_t *parent, size_t psize, const char *path, s3context_t *ctx){
	const char *directory = (const char *) dirname((char *) path);
	//const char *base = (const char *) basename((char *) path); //ease of use

	//putting entry for entry in parent's array
	//size_t psize = parent->metadata->st_size; //size of first entry in parent directory, the "." entry
	s3dirent_t *temp = malloc(psize + sizeof(s3dirent_t));
	
	//copying over old values. Inefficient to grow array by one at a time, but that's what you wanted: Muwhaha.
	int i = 0;
	for(;i<psize/sizeof(s3dirent_t);i++){
		temp[i]=parent[i];
	}

	temp[i] = *entry; //only to let us know it's their. Not a pointer or anything.

	strncpy((char *)temp->name, (char *)parent->name, 256);
	temp->type = parent->type;

	temp->metadata = parent->metadata; //steal parent's metadata struct
	//temp->metadata->st_size = (psize + sizeof(s3dirent_t));
	time(&temp->metadata->st_mtime); //updating modification time for parent directory

	s3fs_remove_object(ctx->s3bucket, directory); //kicking and replacing old directory in sequence
	s3fs_put_object(ctx->s3bucket, directory, (uint8_t*) temp, (psize + sizeof(s3dirent_t)));
}

void shrinkDir(s3dirent_t *parent, size_t psize, const char *path, s3context_t *ctx){
	/*
	//or, more simply
	tgtdir->type = 'u'; //have to input it

	int i = 0;
	for(;i < (psize/sizeof(s3dirent_t));i++){
		if(strcmp(temp[i].name,base)!==0){ //not the same name
			temp[i].type = 'u';
			break;
		}
	}
	return 0;
	*/	//I don't use it because it's simplicity makes me sad and because it throws off size calculations

	const char *directory = (const char *) dirname((char *) path);
	const char *base = (const char *) basename((char *) path);

	//pulling tgtdir from parent's array
	//size_t psize = parent->metadata->st_size; //size stored in first entry in parent directory, the "." entry
	s3dirent_t *temp = malloc(psize - sizeof(s3dirent_t));
	
	//copying over old values.
	int i = 0;
	int k = 0;
	for(;i < (psize/sizeof(s3dirent_t) -1);i++){
		if(strcmp(temp[i].name,base)!=0){ //not the same name
			temp[k]=parent[i];
			k++; //lag behind parent after we hit the dying directory
		}
	}

	strncpy((char *)temp->name, (char *)parent->name, 256);
	temp->type = parent->type;

	temp->metadata = parent->metadata; //steal parent's metadata struct
	//temp->metadata->st_size = (psize - sizeof(s3dirent_t));
	time(&temp->metadata->st_mtime); //updating modification time for parent directory

	s3fs_remove_object(ctx->s3bucket, directory); //kicking and replacing old directory in sequence
	s3fs_put_object(ctx->s3bucket, directory, (uint8_t*) temp, (psize - sizeof(s3dirent_t)));

	s3fs_remove_object(ctx->s3bucket, path); //removing target directory
}

int getObjectIndex(s3dirent_t *parent, size_t par_size, char *childname){
	int i = 1;
	for(;i<par_size/sizeof(s3dirent_t);i++){
		if(strcmp(parent[i].name,childname)){
			return i;
		}
	}
	return -1;
}


/* 
 * Get file attributes.  Similar to the stat() call
 * (and uses the same structure).  The st_dev, st_blksize,
 * and st_ino fields are ignored in the struct (and 
 * do not need to be filled in).
 */

int fs_getattr(const char *path, struct stat *statbuf) {
    fprintf(stderr, "fs_getattr(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
	s3dirent_t *curdir = NULL;
	int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) curdir, 0, 0);
	if(curdir->type == 'd'){
		curdir->metadata->st_size = rs; //don't keep track of size otherwise
		statbuf = curdir->metadata;
	}else{ // a file
		s3dirent_t *parent = NULL;
		int par_size = s3fs_get_object(ctx->s3bucket, (const char *) (dirname((char *) path)), (uint8_t**) parent, 0, 0);

		int file_dir = getObjectIndex(parent, par_size, basename((char *) path));
		parent[file_dir].metadata->st_size = rs;
		statbuf = parent[file_dir].metadata;
	}

	
	return 0; //assume that opendir was successfully called before this, so no problems will be encountered
    //return -EIO;
}


/* 
 * Create a file "node".  When a new file is created, this
 * function will get called.  
 * This is called for creation of all non-directory, non-symlink
 * nodes.  You *only* need to handle creation of regular
 * files here.  (See the man page for mknod (2).)
 */
int fs_mknod(const char *path, mode_t mode, dev_t dev) {
    fprintf(stderr, "fs_mknod(path=\"%s\", mode=0%3o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;

	if(strlen(basename((char *) path))>255){
		return -ENAMETOOLONG;
	}
	
	s3dirent_t *parent = NULL;
	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) dirname((char *) path), (uint8_t**) parent, 0, 0); //grabbing parent directory
	//we're assuming that parent exists

	s3dirent_t *newfile_entry = malloc(sizeof(s3dirent_t)); //entry for array of parent directory
	strncpy((char *)newfile_entry->name, (const char *) basename((char *) path), 256); //only copy first 256 bytes of name, if too long
	newfile_entry->type = 'f';
	
	newfile_entry->metadata->st_mode = mode; //protection
	newfile_entry->metadata->st_uid = getuid(); //user ID of owner
	//newfile_entry->metadata->st_size = 0; //total size of file, in bytes. empty now
	time(&newfile_entry->metadata->st_atime); //time of last access
	time(&newfile_entry->metadata->st_mtime);  //time of last modification
	time(&newfile_entry->metadata->st_ctime); //time of last status change - Read: create

	newfile_entry->metadata->st_nlink = 1; //number of hard links - should be 1
	newfile_entry->metadata->st_gid = getgid(); //group ID of owner

	//s3fs_put_object(ctx->s3bucket, path, (uint8_t*) newfile_entry, 0);

	growDir(newfile_entry,parent, par_size,path, ctx);

	s3fs_put_object(ctx->s3bucket, path, (uint8_t*) mknod(path,mode,dev), sizeof(int));	

	return 0;
    //return -EIO;
}

/* 
 * Create a new directory.
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits (for setting in the metadata)
 * use mode|S_IFDIR.
 */
int fs_mkdir(const char *path, mode_t mode) {
	fprintf(stderr, "fs_mkdir(path=\"%s\", mode=0%3o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    mode |= S_IFDIR;

	if(strlen(basename((char *) path))>255){
		return -ENAMETOOLONG;
	}

	s3dirent_t *curdir = NULL;
	if(s3fs_get_object(ctx->s3bucket, path, (uint8_t**) curdir, 0, 0) != -1){ //object already exists
		return -EEXIST;
	}

	s3dirent_t *parent = NULL;
	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) dirname((char *) path), (uint8_t**) parent, 0, 0); //grabbing parent directory
	//we're assuming that parent exists

	s3dirent_t *newdir = malloc(sizeof(s3dirent_t)); //instantiating new directory
	strcpy(newdir->name, ".");
	newdir->type = 'd';

	newdir->metadata->st_mode = mode; //protection
	newdir->metadata->st_uid = getuid(); //user ID of owner
	//newdir->metadata->st_size = sizeof(s3dirent_t); //total size, in bytes
	time(&newdir->metadata->st_atime); //time of last access
	time(&newdir->metadata->st_mtime);  //time of last modification
	time(&newdir->metadata->st_ctime); //time of last status change - Creation

	newdir->metadata->st_nlink = 1; //number of hard links - should be 1
	newdir->metadata->st_gid = getgid(); //group ID of owner

	s3fs_put_object(ctx->s3bucket, path, (uint8_t*) newdir, sizeof(s3dirent_t));

	s3dirent_t *newdir_entry = malloc(sizeof(s3dirent_t)); //entry for array of parent directory
	strncpy((char *)newdir_entry->name, (const char *) basename((char *) path), 256); //only copy first 256 bytes of name, if too long
	newdir_entry->type = 'd';
	//metadata within the actual (".") entry floating about on the cloud in its array

	growDir(newdir,parent, par_size,path,ctx);

	return 0;
}

/*
 * Remove a file.
 */
int fs_unlink(const char *path) {
    fprintf(stderr, "fs_unlink(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;

	//s3dirent_t *tgtdir, s3dirent_t *parent, size_t par_size, const char *path, s3context_t *ctx

	s3dirent_t *parent = NULL;
	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) dirname((char *) path), (uint8_t**) parent, 0, 0); //grabbing parent directory

	shrinkDir(parent, par_size, path, ctx);

	s3fs_remove_object(ctx->s3bucket,path);

	return 0;
    //return -EIO;
}

/*
 * Remove a directory. 
 */
int fs_rmdir(const char *path) {
    fprintf(stderr, "fs_rmdir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
	
	s3dirent_t *tgtdir = NULL; //checking for existence
	int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) tgtdir, 0, 0);
	if(rs == -1){ //doesn't exist
		return -ENOMEM; //not sure if this is the right error message
	}
	if(rs > sizeof(s3dirent_t)){ //more than one entry
		return -1; //no error message, just don't do anything since there's more than just "." in the directory
	} //???

	s3dirent_t *parent = NULL;
	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) dirname((char *) path), (uint8_t**) parent, 0, 0); //grabbing parent directory

	shrinkDir(parent, par_size,path,ctx);	

	return 0;

    //return -EIO;
}

/*
 * Rename a file. Can also change it's directory. Works on directories and files.
 */
int fs_rename(const char *path, const char *newpath) {
    fprintf(stderr, "fs_rename(fpath=\"%s\", newpath=\"%s\")\n", path, newpath);
    s3context_t *ctx = GET_PRIVATE_DATA;

	if(strlen(basename((char *) path))>255){
		return -ENAMETOOLONG;
	}

	s3dirent_t *old_parent = NULL;
	int old_par_size = s3fs_get_object(ctx->s3bucket, (const char *) dirname((char *) path), (uint8_t**) old_parent, 0, 0); //grabbing parent directory

	s3dirent_t *tgt = NULL;
	int tgt_size = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) tgt, 0, 0);

	s3dirent_t *new_parent = old_parent;
	int new_par_size = old_par_size;

	s3dirent_t tgt_shell = old_parent[getObjectIndex(old_parent, old_par_size, basename((char *) path))];

	if(strcmp(dirname((char *) path),dirname((char *) newpath))!=0){ //different parent directory. Keeps code size down
		new_par_size = s3fs_get_object(ctx->s3bucket, (const char *) dirname((char *) newpath), (uint8_t**) new_parent, 0, 0);
	}

	//no difference for directories or files. Just messing with dirent objects
		
	shrinkDir(old_parent,old_par_size,path,ctx);

	strncpy((char *) tgt->name, basename((char *) newpath), 256);
	growDir(tgt,new_parent,new_par_size,newpath, ctx); //swapping out directories holding pointer to tgt directory

	time(&old_parent->metadata->st_mtime);
	time(&new_parent->metadata->st_mtime);
	time(&tgt_shell.metadata->st_mtime);

	s3fs_remove_object(ctx->s3bucket,path);

	s3fs_put_object(ctx->s3bucket, newpath, (uint8_t *) tgt, tgt_size);

	return 0;
	
    //return -EIO;
}

/*
 * Change the permission bits of a file.
 */
int fs_chmod(const char *path, mode_t mode) {
    fprintf(stderr, "fs_chmod(fpath=\"%s\", mode=0%03o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Change the owner and group of a file.
 */
int fs_chown(const char *path, uid_t uid, gid_t gid) {
    fprintf(stderr, "fs_chown(path=\"%s\", uid=%d, gid=%d)\n", path, uid, gid);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Change the size of a file. Assumes a file is given.
 * "newsize" is always going to be 0, according to the PDF, so I'll just use 0 as a constant.
 */
int fs_truncate(const char *path, off_t newsize) {
    fprintf(stderr, "fs_truncate(path=\"%s\", newsize=%d)\n", path, (int)newsize);
    s3context_t *ctx = GET_PRIVATE_DATA;

	uint8_t **tgt = NULL;
	s3fs_get_object(ctx->s3bucket, path, tgt, 0, 0);

	s3fs_remove_object(ctx->s3bucket,path);
	s3fs_put_object(ctx->s3bucket, path, *tgt, 0);

	s3dirent_t *parent = NULL;
	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) dirname((char *) path), (uint8_t**) parent, 0, 0); //grabbing parent directory

	s3dirent_t tgt_shell = parent[getObjectIndex(parent, par_size, basename((char *) path))];
	time(&tgt_shell.metadata->st_mtime);

	return 0;

    //return -EIO;
}

/*
 * Change the access and/or modification times of a file. 
 */
int fs_utime(const char *path, struct utimbuf *ubuf) {
    fprintf(stderr, "fs_utime(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/* 
 * File open operation
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  
 * 
 * Optionally open may also return an arbitrary filehandle in the 
 * fuse_file_info structure (fi->fh).
 * which will be passed to all file operations.
 * (In stages 1 and 2, you are advised to keep this function very,
 * very simple.)
 */
int fs_open(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_open(path\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
	int rs = s3fs_test_bucket(ctx->s3bucket);
	if(rs==0) //bucket exists
		return 0;
	else
		return -1;
    //return -EIO;
}


/* 
 * Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  
 */
int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_read(path=\"%s\", buf=%p, size=%d, offset=%d)\n", path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
	
	s3dirent_t *temp = NULL;
	int file_size = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) temp, 0, 0);

	if(offset>file_size){
		return 0;
	}
	if(offset+size>file_size){
		; //not sure if something should happen. ???
	}

	int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) buf, offset, size); //might error out. Needs testing ???

	s3dirent_t *parent = NULL; //grabbing parent for metadata update to atime
	int par_size = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) parent, 0, 0);

	time(&parent[getObjectIndex(parent, par_size, basename((char *) path))].metadata->st_atime); //updating access time for file

	return rs;
    //return -EIO;
}

/*
 * Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.
 */
int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_write(path=\"%s\", buf=%p, size=%d, offset=%d)\n",
	        path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;

	int *first_half = NULL;

	int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t **) first_half, 0, offset);
	if(rs == -1){
		return -ENOMEM; //file DNE
	}
	
	uint8_t *whole = malloc(sizeof(uint8_t)*size);
	
	int i = 0;
	for(;i<offset;i++){
		whole[i] = first_half[i]; //setting initial, unaltered bytes
	}
	
	for(;i<size;i++){ //same i, pick up where we left off
		if(i+1<(size-offset)){
			whole[offset + i] = (4<<((int) buf[i])) + buf[i+1]; //chars are 4 bits, need to make them int sized
		}else{
			whole[offset+i] = buf[i];
		}
	}
	
	s3fs_remove_object(ctx->s3bucket,path);
	int newsize = s3fs_put_object(ctx->s3bucket, path, (uint8_t *) whole, rs+size);
	//rs+size accounts for growing the file from the middle

	s3dirent_t *parent = NULL; //updating metadata
	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) basename((char *) path), (uint8_t **) parent, 0, 0);

	time(&parent[getObjectIndex(parent, par_size, basename((char *) path))].metadata->st_mtime); //updating mod time

	free(whole);

	return newsize;
    //return -EIO;
}


/* 
 * Possibly flush cached data for one file.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 */
int fs_flush(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_flush(path=\"%s\", fi=%p)\n", path, fi);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.  
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 */
int fs_release(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_release(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return 0;
}

/*
 * Synchronize file contents; any cached data should be written back to 
 * stable storage.
 */
int fs_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_fsync(path=\"%s\")\n", path);

}

/*
 * Open directory
 *
 * This method should check if the open operation is permitted for
 * this directory
 */
int fs_opendir(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_opendir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
	int rs = s3fs_test_bucket(ctx->s3bucket);
	if(rs==0) //bucket exists
		return 0;
	else
		return -1;
    //return -EIO;
}

/*
 * Read directory.  See the project description for how to use the filler
 * function for filling in directory items.
 */
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_readdir(path=\"%s\", buf=%p, offset=%d)\n", path, buf, (int)offset);

    s3context_t *ctx = GET_PRIVATE_DATA;

	s3dirent_t *curdir = NULL;
	int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) curdir, offset, 0);
	time(&curdir->metadata->st_atime); //reading the directoey requires an update to atime

	int i = 0;
	for(;i < rs/ sizeof(s3dirent_t);i++){ //stolen from pdf
		//call filler function to fill in directory name to the supplied buffer
		if(filler(buf,curdir[i].name,NULL,0)!=0){
			return -ENOMEM;
		}
	}

	return 0;
	//return -EIO;
}

/*
 * Release directory.
 */
int fs_releasedir(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_releasedir(path=\"%s\")\n", path);
    //s3context_t *ctx = GET_PRIVATE_DATA;
    return 0;
	//return -EIO;
}

/*
 * Synchronize directory contents; cached data should be saved to 
 * stable storage.
 */
int fs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_fsyncdir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}

/*
 * Initialize the file system.  This is called once upon
 * file system startup.
 */
void *fs_init(struct fuse_conn_info *conn)
{
    fprintf(stderr, "fs_init --- initializing file system.\n");
    s3context_t *ctx = GET_PRIVATE_DATA;
	s3fs_clear_bucket(ctx->s3bucket); //clearing the bucket

	s3dirent_t *root = malloc(sizeof(s3dirent_t));
	strcpy(root->name, ".");
	root->type = 'd';

	root->metadata->st_mode = 755;
	root->metadata->st_uid = getuid();
	//root->metadata->st_size = sizeof(s3dirent_t);
	time(&root->metadata->st_atime);
	time(&root->metadata->st_mtime);
	time(&root->metadata->st_ctime);

	root->metadata->st_nlink = 1;
	root->metadata->st_gid = getgid();
	

	/* Only using:
	
    mode_t    st_mode;    //protection
    uid_t     st_uid;     //user ID of owner
    time_t    st_atime;   //time of last access
    time_t    st_mtime;   //time of last modification

	nlink_t   st_nlink;   //number of hard links - should be 1
	gid_t     st_gid;     //group ID of owner
	time_t    st_ctime;   //time of last status change - Read: Create time
	*/
	
	s3fs_put_object(ctx->s3bucket, "/", (uint8_t*) root, sizeof(s3dirent_t));

	return ctx;
}

/*void rec_dir_free(s3context_t *ctx, char *path){

	
	s3dirent_t *curdir;
	int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) curdir, 0, 0);
	s3dirent_t **newobject;
	char stuff[10];
	int i = 1;
	for(; i < (curdir[0]->size) / sizeof(s3dirent_t);i++){
		printf("trying to create child\n");
		fflush(stdout);
		printf("newsize = %s %d\n", curdir[0]->name, strlen(curdir[0]->name) + strlen(path) + 1);
		//newdest = malloc((strlen(curdir[0]->name) + strlen(path) + 1) * sizeof(char));
		strcpy(newdest, path);
		printf("newdest = %s\n", newdest);
		strcat(newdest, curdir[0]->name);
		printf("newdest = %s\n", newdest);
		fflush(stdout);
		return;
		s3fs_get_object(ctx->s3bucket, curdir[i]->name, (uint8_t**) newobject, 0, 0);
		if((*newobject)->type == 'd'){
			rec_dir_free(ctx, newdest);
		}
		
		//free(newdest);
		//free(curdir[i]);
	//}
	//free(*curdir);
	printf("This bs is done\n");
}*/

/*
 * Clean up filesystem -- free any allocated data.
 * Called once on filesystem exit.
 */
void fs_destroy(void *userdata) {
    fprintf(stderr, "fs_destroy --- shutting down file system.\n");
	s3context_t *ctx = GET_PRIVATE_DATA;
	//rec_dir_free(userdata, "/"); //yargh!!!!!!!!!
	s3fs_clear_bucket(ctx->s3bucket);
	free(ctx); //same as userdata
}

/*
 * Check file access permissions.  For now, just return 0 (success!)
 * Later, actually check permissions (don't bother initially).
 */
int fs_access(const char *path, int mask) {
    fprintf(stderr, "fs_access(path=\"%s\", mask=0%o)\n", path, mask);
    s3context_t *ctx = GET_PRIVATE_DATA;

	return 0; //complicated, might do if I have the time

	s3dirent_t *parent = NULL;
	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) dirname((char *) path), (uint8_t**) parent, 0, 0);

	s3dirent_t tgt_shell = parent[getObjectIndex(parent, par_size, basename((char *) path))];

	int mode = tgt_shell.metadata->st_mode;
	
	if((mode & mask) == mode){ //all permission bits for tgt fulfilled
    	return 0;
	}
	errno = EACCES;
	return -1; //failed
}

/*
 * Change the size of an open file.  Very similar to fs_truncate (and,
 * depending on your implementation), you could possibly treat it the
 * same as fs_truncate.
 */
int fs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_ftruncate(path=\"%s\", offset=%d)\n", path, (int)offset);
    //s3context_t *ctx = GET_PRIVATE_DATA;

	return fs_truncate(path, offset); //no need to duplicate code, they operate identically

    //return -EIO;
}

/*
 * The struct that contains pointers to all our callback
 * functions.  Those that are currently NULL aren't 
 * intended to be implemented in this project.
 */
struct fuse_operations s3fs_ops = {
  .getattr     = fs_getattr,    // get file attributes
  .readlink    = NULL,          // read a symbolic link
  .getdir      = NULL,          // deprecated function
  .mknod       = fs_mknod,      // create a file
  .mkdir       = fs_mkdir,      // create a directory
  .unlink      = fs_unlink,     // remove/unlink a file
  .rmdir       = fs_rmdir,      // remove a directory
  .symlink     = NULL,          // create a symbolic link
  .rename      = fs_rename,     // rename a file
  .link        = NULL,          // we don't support hard links
  .chmod       = fs_chmod,      // change mode bits
  .chown       = fs_chown,      // change ownership
  .truncate    = fs_truncate,   // truncate a file's size
  .utime       = fs_utime,      // update stat times for a file
  .open        = fs_open,       // open a file
  .read        = fs_read,       // read contents from an open file
  .write       = fs_write,      // write contents to an open file
  .statfs      = NULL,          // file sys stat: not implemented
  .flush       = fs_flush,      // flush file to stable storage
  .release     = fs_release,    // release/close file
  .fsync       = fs_fsync,      // sync file to disk
  .setxattr    = NULL,          // not implemented
  .getxattr    = NULL,          // not implemented
  .listxattr   = NULL,          // not implemented
  .removexattr = NULL,          // not implemented
  .opendir     = fs_opendir,    // open directory entry
  .readdir     = fs_readdir,    // read directory entry
  .releasedir  = fs_releasedir, // release/close directory
  .fsyncdir    = fs_fsyncdir,   // sync dirent to disk
  .init        = fs_init,       // initialize filesystem
  .destroy     = fs_destroy,    // cleanup/destroy filesystem
  .access      = fs_access,     // check access permissions for a file
  .create      = NULL,          // not implemented
  .ftruncate   = fs_ftruncate,  // truncate the file
  .fgetattr    = NULL           // not implemented
};



/* 
 * You shouldn't need to change anything here.  If you need to
 * add more items to the filesystem context object (which currently
 * only has the S3 bucket name), you might want to initialize that
 * here (but you could also reasonably do that in fs_init).
 */
int main(int argc, char *argv[]) {
    // don't allow anything to continue if we're running as root.  bad stuff.
    if ((getuid() == 0) || (geteuid() == 0)) {
    	fprintf(stderr, "Don't run this as root.\n");
    	return -1;
    }
    s3context_t *stateinfo = malloc(sizeof(s3context_t));
    memset(stateinfo, 0, sizeof(s3context_t));

    char *s3key = getenv(S3ACCESSKEY);
    if (!s3key) {
        fprintf(stderr, "%s environment variable must be defined\n", S3ACCESSKEY);
    }
    char *s3secret = getenv(S3SECRETKEY);
    if (!s3secret) {
        fprintf(stderr, "%s environment variable must be defined\n", S3SECRETKEY);
    }
    char *s3bucket = getenv(S3BUCKET);
    if (!s3bucket) {
        fprintf(stderr, "%s environment variable must be defined\n", S3BUCKET);
    }
    strncpy((*stateinfo).s3bucket, s3bucket, BUFFERSIZE);

    fprintf(stderr, "Initializing s3 credentials\n");
    s3fs_init_credentials(s3key, s3secret);

    fprintf(stderr, "Totally clearing s3 bucket\n");
    s3fs_clear_bucket(s3bucket);

    fprintf(stderr, "Starting up FUSE file system.\n");
    int fuse_stat = fuse_main(argc, argv, &s3fs_ops, stateinfo);
    fprintf(stderr, "Startup function (fuse_main) returned %d\n", fuse_stat);
    
    return fuse_stat;
}
