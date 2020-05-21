#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>


#ifdef FS
#include <fs.h>

static struct fsystem fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];

#define SB_BLK 0
#define BM_BLK 1
#define RT_BLK 2

#define NUM_FD 16
struct filetable oft[NUM_FD]; // open file table
int next_open_fd = 0;


#define INODES_PER_BLOCK (fsd.blocksz / sizeof(struct inode))
#define NUM_INODE_BLOCKS (( (fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock);

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock) {
  int diskblock;

  if (fileblock >= INODEBLOCKS - 2) {
    printf("No indirect block support\n");
    return SYSERR;
  }

  diskblock = oft[fd].in.blocks[fileblock]; //get the logical block address

  return diskblock;
}

/* read in an inode and fill in the pointer */
int fs_get_inode_by_num(int dev, int inode_number, struct inode *in) {
  int bl, inn;
  int inode_off;

  if (dev != 0) {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    printf("fs_get_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  inode_off = inn * sizeof(struct inode);

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  printf("inn*sizeof(struct inode): %d\n", inode_off);
  */

  bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
  memcpy(in, &block_cache[inode_off], sizeof(struct inode));

  return OK;

}

/* write inode indicated by pointer to device */
int fs_put_inode_by_num(int dev, int inode_number, struct inode *in) {
  int bl, inn;

  if (dev != 0) {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    printf("fs_put_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  */

  bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
  memcpy(&block_cache[(inn*sizeof(struct inode))], in, sizeof(struct inode));
  bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

  return OK;
}
     
/* create file system on device; write file system block and block bitmask to
 * device */
int fs_mkfs(int dev, int num_inodes) {
  int i;
  
  if (dev == 0) {
    fsd.nblocks = dev0_numblocks;
    fsd.blocksz = dev0_blocksize;
  }
  else {
    printf("Unsupported device\n");
    return SYSERR;
  }

  if (num_inodes < 1) {
    fsd.ninodes = DEFAULT_NUM_INODES;
  }
  else {
    fsd.ninodes = num_inodes;
  }

  i = fsd.nblocks;
  while ( (i % 8) != 0) {i++;}
  fsd.freemaskbytes = i / 8; 
  
  if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *)SYSERR) {
    printf("fs_mkfs memget failed.\n");
    return SYSERR;
  }
  
  /* zero the free mask */
  for(i=0;i<fsd.freemaskbytes;i++) {
    fsd.freemask[i] = '\0';
  }
  
  fsd.inodes_used = 0;
  
  /* write the fsystem block to SB_BLK, mark block used */
  fs_setmaskbit(SB_BLK);
  bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(struct fsystem));
  
  /* write the free block bitmask in BM_BLK, mark block used */
  fs_setmaskbit(BM_BLK);
  bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

  return 1;
}

/* print information related to inodes*/
void fs_print_fsd(void) {

  printf("fsd.ninodes: %d\n", fsd.ninodes);
  printf("sizeof(struct inode): %d\n", sizeof(struct inode));
  printf("INODES_PER_BLOCK: %d\n", INODES_PER_BLOCK);
  printf("NUM_INODE_BLOCKS: %d\n", NUM_INODE_BLOCKS);
}

/* specify the block number to be set in the mask */
int fs_setmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
  return OK;
}

/* specify the block number to be read in the mask */
int fs_getmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return( ( (fsd.freemask[mbyte] << mbit) & 0x80 ) >> 7);
  return OK;

}

/* specify the block number to be unset in the mask */
int fs_clearmaskbit(int b) {
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

fsd.freemask[mbyte] &= invb;
  return OK;
}

/* This is maybe a little overcomplicated since the lowest-numbered
   block is indicated in the high-order bit.  Shift the byte by j
   positions to make the match in bit7 (the 8th bit) and then shift
   that value 7 times to the low-order bit to print.  Yes, it could be
   the other way...  */
void fs_printfreemask(void) { // print block bitmask
  int i,j;

  for (i=0; i < fsd.freemaskbytes; i++) {
    for (j=0; j < 8; j++) {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    if ( (i % 8) == 7) {
      printf("\n");
    }
  }
  
}


int fs_open(char *filename, int flags) {
  if(strlen(filename) >FILENAMELEN) {
    kprintf("File name length exceeded\n");
 		return SYSERR;
 	}
  
  // if open file table is empty
  if(next_open_fd > 0){
    
    for(int i = 0; i<next_open_fd; i++){
      if(strcmp(oft[i].de->name,filename) == 0){

        //Check if the file is already open
        if(oft[i].state == FSTATE_OPEN){
          kprintf("File is already open\n");
          return SYSERR;
        }
        oft[i].flag = flags;
        oft[i].state = FSTATE_OPEN;
        
        return i; // return index
      }
    }
    return SYSERR;
  }
  return SYSERR;

}

int fs_close(int fd) {
  
  if(fd> NUM_FD || fd<0){
    return SYSERR;
  }
  if(oft[fd].state == FSTATE_OPEN){
      oft[fd].state = FSTATE_CLOSED; //FSTATE_CLOSED
      return OK;
  }else{
      kprintf("File already closed \n");
      return SYSERR; 
  }
  
}

int fs_create(char *filename, int mode) {
    if(strlen(filename) >FILENAMELEN) {
      kprintf("File name length exceeded\n");
      return SYSERR;
 	}
    if(mode == O_CREAT){
    int i=0;
    if(next_open_fd > 0){ // if file already exists
        for(i = 0; i<next_open_fd; i++){
          if(strcmp(oft[i].de->name,filename)==0){
            printf("Filename exists\n");
            return SYSERR;
          }
        }
      } 
      if(next_open_fd >= NUM_FD){ // filetable if full 
        kprintf("File table is full\n");
        return SYSERR;
      }
      if(fsd.ninodes == fsd.inodes_used){ // if inodes exists
        kprintf("No inodes exists\n");
        return SYSERR;
      }
      oft[next_open_fd].fileptr = 0;
      oft[next_open_fd].state = FSTATE_CLOSED;
      oft[next_open_fd].de = getmem(sizeof(struct dirent));
      strcpy(oft[next_open_fd].de->name, filename);
      oft[next_open_fd].in.id = fsd.inodes_used;
      oft[next_open_fd].de->inode_num = fsd.inodes_used;
      
      int enteries = fsd.root_dir.numentries++;
      fsd.root_dir.entry[enteries] = *oft[next_open_fd].de;
      oft[next_open_fd].in.nlink = 1;
      oft[next_open_fd].in.device = 0;
      oft[next_open_fd].in.size = 0;
      oft[next_open_fd].in.type = INODE_TYPE_FILE;
      fs_put_inode_by_num(0, oft[next_open_fd].in.id, &oft[next_open_fd].in);
      int fd = next_open_fd;
      next_open_fd++;
      fsd.inodes_used++;
      
    return fs_open(filename, O_RDWR);;
  }else{
     kprintf("Invalid file create mode\n");
     return SYSERR;
  }
  
}

int fs_seek(int fd, int offset) {
  if(fd> NUM_FD || fd<0){
    return SYSERR;
  }
  
  if(strlen(oft[fd].de->name) == 0) {
    kprintf("File not present\n");
 		return SYSERR;
 	}
  
  int fileptr = oft[fd].fileptr;
  if(oft[fd].state==FSTATE_CLOSED){
    return SYSERR;
  }

  oft[fd].fileptr += offset;
  return OK;
}

int fs_read(int fd, void *buf, int nbytes) {
  
  //check mode 
  if(oft[fd].flag == O_WRONLY){
      printf("Access denied\n");
      return SYSERR;
  }

  //file is closed
  if(oft[fd].state == FSTATE_CLOSED){
      printf("File closed\n");
      return SYSERR;
  }

 
 int num_blocks = (nbytes/MDEV_BLOCK_SIZE) + 1;
 int start_block = oft[fd].fileptr/MDEV_BLOCK_SIZE;
 int start_block_address = oft[fd].fileptr;
 int i = start_block;
 int inode_number = oft[fd].de->inode_num;
 struct inode *in = getmem(sizeof(struct inode));
 fs_get_inode_by_num(0,inode_number,in);
 int total_bytes = nbytes;
 int offset = oft[fd].fileptr-start_block_address;
 int block=0, left, k = 0;

 while( > 0){

  block=in->blocks[i];

  if(k==0){

    if(MDEV_BLOCK_SIZE -(oft[fd].fileptr % MDEV_BLOCK_SIZE) < nbytes){
      left = MDEV_BLOCK_SIZE - (oft[fd].fileptr % MDEV_BLOCK_SIZE);
      bs_bread(0, block, offset,block_cache, left);
      memcpy(buf,block_cache,left);
      nbytes -= left;
      offset += left;
    }
    else{
      bs_bread(0, block, offset,block_cache, nbytes);
      memcpy(buf,block_cache,left);
      offset += nbytes;
      nbytes=0;
    }
    k++;
  }
  else{
    if(nbytes > MDEV_BLOCK_SIZE){
      bs_bread(0, block, 0,block_cache, MDEV_BLOCK_SIZE);
      strncat(buf,block_cache,MDEV_BLOCK_SIZE);
      nbytes -= MDEV_BLOCK_SIZE;
      offset += MDEV_BLOCK_SIZE;
    }
    else{
      bs_bread(0, block, 0,block_cache, nbytes);
      strncat(buf,block_cache,nbytes);
      nbytes = 0;
    }
  }
  i++;
  k++;
}

 oft[fd].fileptr += total_bytes;
 return oft[fd].fileptr;
 
}

int fs_write(int fd, void *buf, int nbytes) {
  //file is already open
  if(fd> NUM_FD || fd<0){
    return SYSERR;
  }
  if(oft[fd].state != FSTATE_OPEN ){
    kprintf("File is not open\n");
    return SYSERR;
  }

  if(oft[fd].flag == O_RDONLY){
    kprintf("Access denied\n");
    return SYSERR;
  }
  if(strlen(oft[fd].de->name) == 0 ) {
    printf("\nFile not present");
    return SYSERR;
  }
  int inode_id = oft[fd].in.id;
  struct inode* in = (struct inode*)getmem(sizeof(struct inode));
  fs_get_inode_by_num(0, oft[fd].de->inode_num, in);
  char* buffer = getmem(fsd.blocksz);
 
  int required_blocks = nbytes/MDEV_BLOCK_SIZE;
  if(nbytes % MDEV_BLOCK_SIZE != 0) {
    required_blocks++;
  }

  int prev_offset = nbytes%MDEV_BLOCK_SIZE;
  int i,j;

  for (i = 0;i < required_blocks; i++) {
    void* offset = buf+i*MDEV_BLOCK_SIZE;
    for(j = NUM_INODE_BLOCKS + FIRST_INODE_BLOCK; j < MDEV_NUM_BLOCKS; j++) {
      if(fs_getmaskbit(j) == 0) {
        fs_setmaskbit(j);

        if(i == required_blocks-1) {
          memcpy((void*)buffer, offset, prev_offset);
          bs_bwrite(0, j, 0, (void*)buffer, fsd.blocksz);
          in->blocks[i] = j;
        }
        else{
          memcpy((void*)buffer, offset, MDEV_BLOCK_SIZE);
          bs_bwrite(0, j, 0, buffer, fsd.blocksz);  
          in->blocks[i] = j;
        }
        break;
      }
    }
  }

  fs_put_inode_by_num(0, inode_id , in);
  oft[fd].in=*in;
  oft[fd].fileptr+=nbytes;

  return nbytes;
}



int fs_link(char *src_filename, char* dst_filename) {	  
   int i;	
  if(strlen(src_filename) >FILENAMELEN) {
    kprintf("File name length exceeded\n");
 		return SYSERR;
 	}
   if(strlen(dst_filename) >FILENAMELEN) {
    kprintf("File name length exceeded\n");
 		return SYSERR;
 	}
  if(next_open_fd > 0){ // if file already exists
          	         
        for(i = 0; i<next_open_fd; i++){	         
          if(strcmp(oft[i].de->name,dst_filename)==0){	
            printf(" Destination filename already exists\n");	
            return SYSERR;	
          }	
          if(strcmp(oft[i].de->name,src_filename) == 0){	
            // srcfile found 	
            int inode_number = oft[i].de->inode_num;	
            fs_get_inode_by_num(0,inode_number,&oft[i].in);	
            oft[i].in.nlink +=1 ;	
            oft[next_open_fd].de->inode_num = inode_number;
            fs_put_inode_by_num(0, inode_number, &oft[i].in);	
            strcpy(oft[next_open_fd].de->name,dst_filename);	
            next_open_fd++;	
            fsd.root_dir.numentries++;	
            return OK;	
          }	
        }
        kprintf("Src file not found\n");
        return SYSERR;
    }
    return SYSERR;
}


int fs_unlink(char *filename) {
  int i;
  if(next_open_fd > 0) { 
  for(i = 0; i<next_open_fd; i++){
      if(strcmp(oft[i].de->name,filename) == 0){
        // file found 
        int inode_number = oft[i].de->inode_num;
        struct inode *in = getmem(sizeof(struct inode));
        fs_get_inode_by_num(0,inode_number,&oft[i].in);
        if(oft[i].in.nlink==1){
              int j = oft[i].fileptr/MDEV_BLOCK_SIZE;
              int nbytes = 1200;
              int block = 0;
              while(nbytes > 0){
                block = oft[i].in.blocks[j];
                fs_clearmaskbit(block);
                nbytes -= MDEV_BLOCK_SIZE;
                j++;
              }
              fsd.inodes_used--;
        }else{
           in->nlink--;
        }
        next_open_fd--;
        fsd.root_dir.numentries--;
        return OK;
      }
  }
  kprintf("File does not exist");
  return SYSERR;
  }
  kprintf("No files");
  return SYSERR;
}
#endif /* FS */
