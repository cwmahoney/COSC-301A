/* This code is based on the fine code written by Joseph Pfeiffer for his
   fuse system tutorial. */
/* If any more memory leaks then fix all put_objects. */

/* When adding directories, they should have a / at the end!! */

/*
Curtis Mahoney & Adriana Sperlea
12/7/2012
Purpose: Implement an AmazonS3 interface for cloud-based file actions
-Note: Debugging printf's are left in on purpose, as end-user will not see them unless in -d mode.

*/

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

void growDir(s3dirent_t entry, s3dirent_t *parent, size_t psize, const char *path, s3context_t *ctx){
	printf("Growing directory %s with of size %ld with entry %s and path %s\n", parent->name, psize, entry.name, path);
	char *ptemp1 = strdup(path);
	char *ptemp2 = strdup(path);
	char *directory = strdup(dirname(ptemp1));
	char *base = strdup(basename(ptemp2));

	free(ptemp1);
	free(ptemp2);

	//putting entry for entry in parent's array
	//size_t psize = parent->metadata.st_size; //size of first entry in parent directory, the "." entry
	s3dirent_t *temp = malloc(psize + sizeof(s3dirent_t));
	
	//copying over old values. Inefficient to grow array by one at a time, but that's what you wanted: Muwhaha.
	int i = 0;
	for(;i<psize/sizeof(s3dirent_t);i++){
		temp[i]=parent[i];
	}

	temp[i] = entry; //only to let us know it's their. Not a pointer or anything.

	strcpy((char *)temp->name, (char *)parent->name);
	temp->type = parent->type;

	temp->metadata = parent->metadata; //steal parent's metadata struct
	time(&temp->metadata.st_mtime); //updating modification time for parent directory

	s3fs_remove_object(ctx->s3bucket, directory); //kicking and replacing old directory in sequence
	printf("size of what we're putting up is %d/%d = %ld\n",psize + sizeof(s3dirent_t),sizeof(s3dirent_t),(psize + sizeof(s3dirent_t))/sizeof(s3dirent_t));
	s3fs_put_object(ctx->s3bucket, directory, (uint8_t*) temp, (psize + sizeof(s3dirent_t)));
	free(directory);
	free(base);
	free(temp);
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

	char * ptemp = strdup(path);
	char *ptemp2;
	ptemp2 = dirname(ptemp);
	char directory[strlen(ptemp2)];
	strcpy(directory,ptemp2);

	strcpy(ptemp,path);
	ptemp2 = basename(ptemp);
	char base[strlen(ptemp2)];
	strcpy(base,ptemp2);

	free(ptemp);

	//pulling tgtdir from parent's array
	//size_t psize = parent->metadata.st_size; //size stored in first entry in parent directory, the "." entry
	printf("sizes: %d %d\n", psize, psize - sizeof(s3dirent_t));
	s3dirent_t *temp = malloc(psize - sizeof(s3dirent_t));
	
	//copying over old values.
	int i = 0;
	int k = 0;
	//printf("!!!!!!!!!!!!!size = %d !!!!!!!!!!!!!!!\n",(psize/sizeof(s3dirent_t)));
	for(;i < (psize/sizeof(s3dirent_t));i++){
		if(strcmp((char *) parent[i].name, base)!=0){ //not the same name
			temp[k]=parent[i];
			printf("%s\n", temp[k].name);
			k++; //lag behind parent after we hit the tgt entry
			//printf("Copied: ");
		}
		//printf("Old name: %s\n",temp[i].name);
	}
	
	strcpy((char *)temp->name, (char *)parent->name);

	temp->type = parent->type;

	//temp->metadata = parent->metadata; //steal parent's metadata struct

	//experiment
	temp->metadata.st_mode = parent->metadata.st_mode;
	temp->metadata.st_uid = parent->metadata.st_uid;
	temp->metadata.st_ctime = parent->metadata.st_ctime;
	temp->metadata.st_atime = parent->metadata.st_atime;

	temp->metadata.st_nlink = parent->metadata.st_nlink;
	temp->metadata.st_gid = parent->metadata.st_gid;
	

	//temp->metadata.st_size = (psize - sizeof(s3dirent_t));
	time(&temp->metadata.st_mtime); //updating modification time for parent directory

	//printf("Directory: %s\n",directory);

	s3fs_remove_object(ctx->s3bucket, directory); //kicking and replacing old directory in sequence
	printf("When shrinking we're putting back size %d\n", psize - sizeof(s3dirent_t));
	s3fs_put_object(ctx->s3bucket, directory, (uint8_t*) temp, psize - sizeof(s3dirent_t));
	s3fs_remove_object(ctx->s3bucket, path); //removing target object
	free(temp);
}

int getObjectIndex(s3dirent_t *parent, size_t par_size, char *childname){
	int i = 0;
	for(;i<par_size/sizeof(s3dirent_t);i++){
		if(strcmp(parent[i].name,childname) == 0){
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
	char *ptemp1 = strdup(path);
	char *ptemp2 = strdup(path);
	char *directory = strdup(dirname(ptemp1));
	char *base = strdup(basename(ptemp2));

	free(ptemp1);
	free(ptemp2);

    fprintf(stderr, "fs_getattr(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
	s3dirent_t *parent = NULL;

	if (strcmp(directory, base) != 0) { // not in root
		int parent_size = s3fs_get_object(ctx->s3bucket, (const char *) (directory), (uint8_t**) &parent, 0, 0); // get parent array
		
		int file_ind = getObjectIndex(parent, parent_size, base); // find index of object in parent array

		if (file_ind == -1) { // file does not exist
			return -ENOENT;
		}

		if (parent[file_ind].type == 'f') {
			*statbuf = parent[file_ind].metadata; // otherwise get metadata from array
			s3dirent_t *aux = NULL; // used only to get the object size
			statbuf->st_size = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) &aux, 0, 0);
		} else {
			s3dirent_t *aux = NULL; // used to get all directory metadata
			int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) &aux, 0, 0);
			*statbuf = aux->metadata;
			statbuf->st_size = rs;
		}
	} else {
		s3dirent_t *aux = NULL; // used to get all directory metadata
		int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) &aux, 0, 0);
		*statbuf = aux->metadata;
		statbuf->st_size = rs;
	}
		
	free(directory);
	free(base);
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
    fprintf(stderr, "fs_mknod(path=\"%s\", mode=0%3o, dev=\"%s\")\n", path, mode, dev);
    s3context_t *ctx = GET_PRIVATE_DATA;

	char * ptemp = strdup(path);
	char *ptemp2;
	ptemp2 = dirname(ptemp);
	char directory[strlen(ptemp2)];
	strcpy(directory,ptemp2);

	strcpy(ptemp,path);
	ptemp2 = basename(ptemp);
	char base[strlen(ptemp2)];
	strcpy(base,ptemp2);

	free(ptemp);

	if(strlen(base)>255){
		return -ENAMETOOLONG;
	}
	
	s3dirent_t *parent = NULL;
	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) directory, (uint8_t**) &parent, 0, 0); //grabbing parent directory
	//we're assuming that parent exists
	printf("Grabbed parent directory\n");

	s3dirent_t newfile_entry; //entry for array of parent directory
	strcpy((char *)newfile_entry.name, (const char *) base); //only copy first 256 bytes of name, if too long
	newfile_entry.type = 'f';

	newfile_entry.metadata.st_mode = mode; //protection
	newfile_entry.metadata.st_uid = getuid(); //user ID of owner
	//newfile_entry->metadata.st_size = 0; //total size of file, in bytes. empty now
	time(&newfile_entry.metadata.st_atime); //time of last access
	time(&newfile_entry.metadata.st_mtime);  //time of last modification
	time(&newfile_entry.metadata.st_ctime); //time of last status change - Read: create

	newfile_entry.metadata.st_nlink = 1; //number of hard links - should be 1
	newfile_entry.metadata.st_gid = getgid(); //group ID of owner

	//s3fs_put_object(ctx->s3bucket, path, (uint8_t*) newfile_entry, 0);

	growDir(newfile_entry,parent, par_size,path, ctx);
	s3fs_put_object(ctx->s3bucket, path, "", 0);	
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

	char * ptemp = strdup(path);
	char *ptemp2;
	ptemp2 = dirname(ptemp);
	char directory[strlen(ptemp2)];
	strcpy(directory,ptemp2);

	strcpy(ptemp,path);
	ptemp2 = basename(ptemp);
	char base[strlen(ptemp2)];
	strcpy(base,ptemp2);

	free(ptemp);

	if(strlen(base)>255){
		return -ENAMETOOLONG;
	}
	
	s3dirent_t *curdir = NULL;
	if(s3fs_get_object(ctx->s3bucket, path, (uint8_t**) &curdir, 0, 0) != -1){ //object already exists
		return -EEXIST;
	}

	s3dirent_t *parent = NULL;

	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) directory, (uint8_t**) &parent, 0, 0); //grabbing parent directory
	//we're assuming that parent exists

	s3dirent_t *newdir = malloc(sizeof(s3dirent_t)); //instantiating new directory
	strcpy(newdir->name, ".");
	newdir->type = 'd';

	// creating metadata for newdir
	newdir->metadata.st_mode = mode; //protection
	newdir->metadata.st_uid = getuid(); //user ID of owner
	time(&newdir->metadata.st_atime); //time of last access
	time(&newdir->metadata.st_mtime);  //time of last modification
	time(&newdir->metadata.st_ctime); //time of last status change - Creation

	newdir->metadata.st_nlink = 1; //number of hard links - should be 1
	newdir->metadata.st_gid = getgid(); //group ID of owner

	// put object for new directory on s3
	s3fs_put_object(ctx->s3bucket, path, (uint8_t*) newdir, sizeof(s3dirent_t));
	free(newdir);

	s3dirent_t newdir_entry; //entry for array of parent directory
	strcpy((char *)newdir_entry.name, (const char *) base); 
	newdir_entry.type = 'd';
	//metadata within the actual (".") entry floating about on the cloud in its array

	growDir(newdir_entry, parent, par_size, path, ctx);
	//free(newdir_entry);

	return 0;
}

/*
 * Remove a file.
 */
int fs_unlink(const char *path) {
    fprintf(stderr, "fs_unlink(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;

	char * ptemp = strdup(path);
	char *ptemp2;
	ptemp2 = dirname(ptemp);
	char directory[strlen(ptemp2)];
	strcpy(directory,ptemp2);

	free(ptemp);

	//s3dirent_t *tgtdir, s3dirent_t *parent, size_t par_size, const char *path, s3context_t *ctx

	s3dirent_t *parent = NULL;
	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) directory, (uint8_t**) &parent, 0, 0); //grabbing parent directory

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
	
	char * ptemp = strdup(path);
	char *ptemp2;
	ptemp2 = dirname(ptemp);
	char directory[strlen(ptemp2)];
	strcpy(directory,ptemp2);

	free(ptemp);

	s3dirent_t *tgtdir = NULL; //checking for existence
	int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) &tgtdir, 0, 0);
	if(rs == -1){ //doesn't exist
		return -ENOMEM; //not sure if this is the right error message
	}
	if(rs > sizeof(s3dirent_t)){ //more than one entry
		return -1; //no error message, just don't do anything since there's more than just "." in the directory
	} //???

	s3dirent_t *parent = NULL;
	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) directory, (uint8_t**) &parent, 0, 0); //grabbing parent directory

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

	char * ptemp = strdup(path);
	char *ptemp2;
	ptemp2 = dirname(ptemp);
	char directory[strlen(ptemp2)];
	strcpy(directory,ptemp2);

	strcpy(ptemp,path);
	ptemp2 = basename(ptemp);
	char base[strlen(ptemp2)];
	strcpy(base,ptemp2);

	free(ptemp);

	if(strlen(base)>255){
		return -ENAMETOOLONG;
	}

	s3dirent_t *old_parent = NULL;
	int old_par_size = s3fs_get_object(ctx->s3bucket, (const char *) directory, (uint8_t**) &old_parent, 0, 0); //grabbing parent directory
	printf("Parent directory grabbed.\n");

	s3dirent_t *tgt = NULL;
	int tgt_size = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) &tgt, 0, 0);

	s3dirent_t tgt_shell = old_parent[getObjectIndex(old_parent, old_par_size, base)];

	char * nptemp = strdup(newpath);
	char *nptemp2;
	nptemp2 = dirname(nptemp);
	char ndirectory[strlen(nptemp2)];
	strcpy(ndirectory,nptemp2);

	strcpy(nptemp,newpath);
	nptemp2 = basename(nptemp);
	char nbase[strlen(nptemp2)];
	strcpy(nbase,nptemp2);

	free(nptemp);

	

	//no difference for directories or files. Just messing with dirent objects
		
	shrinkDir(old_parent,old_par_size,path,ctx);

	strcpy((char *) tgt_shell.name, nbase);

	s3dirent_t *new_parent;
	int new_par_size;
	new_par_size = s3fs_get_object(ctx->s3bucket, (const char *) ndirectory, (uint8_t**) &new_parent, 0, 0);

	growDir(tgt_shell,new_parent,new_par_size,newpath, ctx); //swapping out directories holding pointer to tgt directory

	time(&old_parent->metadata.st_mtime);
	time(&new_parent->metadata.st_mtime);
	time(&tgt_shell.metadata.st_mtime);

	//s3fs_remove_object(ctx->s3bucket,path);

	s3fs_put_object(ctx->s3bucket, newpath, (uint8_t *) tgt, tgt_size);

	if(tgt_shell.type == 'd'){ // directory, need to recurse down
		//s3context_t *ctx = GET_PRIVATE_DATA; //need to redeclare for some reason
		int i = 0;
		s3fs_put_object(ctx->s3bucket, path, (uint8_t *) tgt, tgt_size); //pushing it up to cloud temporarily so children can see it, refer to it.
		for(;i<tgt_size/sizeof(s3dirent_t);i++){
			if(strcmp(tgt[i].name,".") != 0){
				char *child_oldpath = malloc(strlen(path)+strlen(tgt[i].name) + 1);
				strcpy(child_oldpath, path);
				strcat(child_oldpath, "/");
				strcat(child_oldpath, tgt[i].name);

				char *child_newpath = malloc(strlen(newpath)+strlen(tgt[i].name) +1);
				strcpy(child_newpath, newpath);
				strcat(child_newpath, "/");
				strcat(child_newpath, tgt[i].name);

				fs_rename(child_oldpath,child_newpath);
				free(child_oldpath);
				free(child_newpath);
			}
		}
		s3fs_remove_object(ctx->s3bucket, path); //killing copy of tgt
	}

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

	char *ptemp = strdup(path);
	char *ptemp2;
	ptemp2 = dirname(ptemp);
	char directory[strlen(ptemp2)];
	strcpy(directory,ptemp2);

	strcpy(ptemp,path);
	ptemp2 = basename(ptemp);
	char base[strlen(ptemp2)];
	strcpy(base,ptemp2);

	free(ptemp);

	/*uint8_t **tgt = NULL;
	printf("About to get object\n");
	s3fs_get_object(ctx->s3bucket, path, tgt, 0, 0);*/

	printf("Got object!\n");
	s3fs_remove_object(ctx->s3bucket,path);
	printf("Removed object\n");
	s3fs_put_object(ctx->s3bucket, path, (uint8_t *) "", 0);
	printf("Put object\n");
	s3dirent_t *parent = NULL; //grabbing parent for metadata update to atime
	int par_size = s3fs_get_object(ctx->s3bucket, directory, (uint8_t**) &parent, 0, 0);

	time(&parent[getObjectIndex(parent, par_size, base)].metadata.st_mtime); //updating access time for file

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
	s3dirent_t *curdir;
	int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t **) &curdir, 0, 0);
	//int rs = s3fs_test_bucket(ctx->s3bucket);
	if(rs < 0) //bucket does not exist
		return -1;
	else
		return 0;
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

	char * ptemp = strdup(path);
	char *ptemp2;
	ptemp2 = dirname(ptemp);
	char directory[strlen(ptemp2)];
	strcpy(directory,ptemp2);

	strcpy(ptemp,path);
	ptemp2 = basename(ptemp);
	char base[strlen(ptemp2)];
	strcpy(base,ptemp2);

	free(ptemp);
	
	uint8_t *temp = NULL;
	int file_size = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) &temp, 0, 0);

	if(offset>file_size){
		return 0;
	}
	/*
	if(offset+size>file_size){
		; //not sure if something should happen. ???
	}*/

	int i = offset;
	for(; (i < offset + size) && (i < file_size); i++) {
		buf[i - offset] = temp[i];
	}
	if (i < offset + size) {
		for (; i < offset + size; i++) {
			buf[i - offset] = 0;
		}
	}

	s3dirent_t *parent = NULL; //grabbing parent for metadata update to atime
	int par_size = s3fs_get_object(ctx->s3bucket, directory, (uint8_t**) &parent, 0, 0);

	time(&parent[getObjectIndex(parent, par_size, base)].metadata.st_atime); //updating access time for file

	return size;
    //return -EIO;
}

/*
 * Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.
 */
int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_write(path=\"%s\", buf=%p, size=%d, offset=%d)\n", path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;

	char *ptemp = strdup(path);
	char *base = basename(ptemp);
	free(ptemp);

	uint8_t *first_half = NULL;
 
	int rs = s3fs_get_object(ctx->s3bucket, path, &first_half, 0, offset);
	if(rs == -1){
		return -ENOMEM; //file DNE
	}
	
	uint8_t *whole = malloc(sizeof(uint8_t)*(offset+size));
	uint8_t *goodbuf = (uint8_t *) buf;	

	int i = 0;
	for(;i<offset;i++){
		whole[i] = first_half[i]; //setting initial, unaltered bytes
	}
	printf("whole = %s\n", whole);
	
	for(;i<(offset+size);i++){ //same i, pick up where we left off
		whole[i] = goodbuf[i - offset];
	}
	printf("whole = %s\n", whole);

	uint8_t *second_half = NULL;
	int bigsize = s3fs_get_object(ctx->s3bucket, path, &second_half, 0,0);
	bigsize = s3fs_get_object(ctx->s3bucket, path, &second_half, offset+size,(bigsize-(offset+size)));	

	for(;i<(bigsize);i++){ //same i, pick up where we left off
		whole[i] = second_half[i - (size + offset)];
	}	

	s3fs_remove_object(ctx->s3bucket,path);
	printf("whole = %s\n path = %s\n", whole, path);
	int newsize = s3fs_put_object(ctx->s3bucket, path, (uint8_t *) whole, i);
	//rs+size accounts for growing the file from the middle

	s3dirent_t *parent = NULL; //updating metadata
	int par_size = s3fs_get_object(ctx->s3bucket, path, (uint8_t **) &parent, 0, 0);

	time(&parent[getObjectIndex(parent, par_size, base)].metadata.st_mtime); //updating mod time

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

	return 0; //dirty caching
    //return -EIO;
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
	s3dirent_t *curdir;
	int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t **) &curdir, 0, 0);
	if(rs < 0) //directory does not exist
		return -ENOENT;
	else 
		return 0;
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
	int rs = s3fs_get_object(ctx->s3bucket, path, (uint8_t**) &curdir, offset, 0);
	time(&curdir->metadata.st_atime); //reading the directoey requires an update to atime

	int i = 0;

	
	for(;i < rs / sizeof(s3dirent_t);i++){ //stolen from pdf
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
    return 0;

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
	
	//initializing metadata
	root->metadata.st_mode = (S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR);
	root->metadata.st_uid = getuid();
	time(&root->metadata.st_atime);
	time(&root->metadata.st_mtime);
	time(&root->metadata.st_ctime);

	root->metadata.st_nlink = 1;
	root->metadata.st_gid = getgid();

	s3fs_put_object(ctx->s3bucket, "/", (uint8_t*) root, sizeof(s3dirent_t));

	free(root);
	return ctx;
}

/*
 * Clean up filesystem -- free any allocated data.
 * Called once on filesystem exit.
 */
void fs_destroy(void *userdata) {
    fprintf(stderr, "fs_destroy --- shutting down file system.\n");
	s3context_t *ctx = GET_PRIVATE_DATA;
	s3fs_clear_bucket(ctx->s3bucket);
	free(ctx); 
}

/*
 * Check file access permissions.  For now, just return 0 (success!)
 * Later, actually check permissions (don't bother initially).
 */

int fs_access(const char *path, int mask) {
    fprintf(stderr, "fs_access(path=\"%s\", mask=0%o)\n", path, mask);
    s3context_t *ctx = GET_PRIVATE_DATA;

	char * ptemp = strdup(path);
	char *ptemp2;
	ptemp2 = dirname(ptemp);
	char directory[strlen(ptemp2)];
	strcpy(directory,ptemp2);

	strcpy(ptemp,path);
	ptemp2 = basename(ptemp);
	char base[strlen(ptemp2)];
	strcpy(base,ptemp2);

	free(ptemp);

	return 0; //complicated, might do if I have the time

	s3dirent_t *parent = NULL;
	int par_size = s3fs_get_object(ctx->s3bucket, (const char *) directory, (uint8_t**) &parent, 0, 0);

	s3dirent_t tgt_shell = parent[getObjectIndex(parent, par_size, base)];

	int mode = tgt_shell.metadata.st_mode;
	
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
