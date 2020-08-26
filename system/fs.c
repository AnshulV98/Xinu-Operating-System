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
  printf("\n");
}


int fs_open(char *filename, int flags) {
  if (flags != O_RDONLY && flags != O_WRONLY && flags != O_RDWR){
        printf("Mode Invalid");
        return SYSERR;
    }
    int fd;
    struct inode node;
    int numEntries = fsd.root_dir.numentries;
	if(numEntries <= 0){
		return SYSERR;
	}
    for(fd = 0; fd < numEntries; fd++){
        if (oft[fd].state != FSTATE_OPEN && strcmp(fsd.root_dir.entry[fd].name, filename) == 0){
			int inode_num = fsd.root_dir.entry[fd].inode_num;
            fs_get_inode_by_num(0, inode_num, &node);
			oft[fd].state = FSTATE_OPEN;
			oft[fd].fileptr = 0;
			oft[fd].in = node;
			oft[fd].de = &fsd.root_dir.entry[fd];
			oft[fd].flag = flags;
			fs_put_inode_by_num(0, inode_num, &oft[fd].in);
			return fd;
        }
    }
    return SYSERR;
}

int fs_close(int fd) {
  if(oft[fd].state == FSTATE_CLOSED){
        printf("File already in closed state\n");
        return SYSERR;
    }
    else if(fd < 0 || fd >= NUM_FD){
        printf("Invalid file\n");
        return SYSERR;
    }
    oft[fd].state = FSTATE_CLOSED;
    oft[fd].fileptr = 0;
    return OK;
}

int fs_create(char *filename, int mode) {
  if(mode == O_CREAT && fsd.root_dir.numentries < DIRECTORY_SIZE){
		int fd;
		int inode_num = 0;
		for(fd = 0; fd < fsd.root_dir.numentries; fd++){
			if(strcmp(filename, fsd.root_dir.entry[fd].name) == 0){
				return SYSERR;
			}
		}
		struct inode node;
		inode_num = fsd.inodes_used++;
		fs_get_inode_by_num(0, inode_num, &node);
		node.id = inode_num;
		node.type = INODE_TYPE_FILE;
		node.device = 0;
        node.type = INODE_TYPE_FILE;
		node.size = 0;
		fs_put_inode_by_num(0, node.id, &node);
		fsd.root_dir.entry[fsd.root_dir.numentries].inode_num = node.id;
		strcpy(fsd.root_dir.entry[fsd.root_dir.numentries].name, filename);
		fsd.root_dir.numentries++;
		return fs_open(filename, O_RDWR);
	}
	return SYSERR;
}

int fs_seek(int fd, int offset) {
  if(fd < 0 || fd >= NUM_FD){
        return SYSERR;
    }
    int fileptr = oft[fd].fileptr + offset;
    if(fileptr > oft[fd].in.size){
        return SYSERR;
    }
    oft[fd].fileptr = fileptr;
    return OK;
}

int fs_read(int fd, void *buf, int nbytes) {
  struct filetable ft = oft[fd];
    struct inode node = ft.in;
    int bytes, original_nbytes = nbytes;
    int inode_block = (ft.fileptr / fsd.blocksz);
    int inode_offset = (ft.fileptr % fsd.blocksz);

    if(oft[fd].state == FSTATE_CLOSED){
        printf("File in closed state\n");
        return SYSERR;
    }
    else if(fd < 0 || fd > NUM_FD){
        printf("Invalid File\n");
        return SYSERR;
    }
    else if(nbytes <= 0){
        printf("Input buffer length is less than 0\n");
        return SYSERR;
    }
    if(inode_block < INODEBLOCKS){
        int in_blk = ft.in.blocks[inode_block];
        int new_blocks = fsd.blocksz - inode_offset;
        while(nbytes > 0){
            if(new_blocks <= nbytes){
                if(inode_block == INODEBLOCKS - 1){
                    printf("FS_READbByte limit exceeded\n");
                    bytes = original_nbytes - nbytes;
                    return bytes;
                }
                bs_bread(0, in_blk, inode_offset, buf, new_blocks);
                ft.fileptr = ft.fileptr + new_blocks;
                buf = buf + new_blocks;
                nbytes = nbytes - new_blocks;
                memcpy(oft + fd, &ft, sizeof(struct filetable));
                in_blk = node.blocks[++inode_block];
                inode_offset = 0;
            }
            else if(new_blocks > nbytes){
                bs_bread(0, in_blk, inode_offset, buf, nbytes);
                buf = buf + nbytes;
                ft.fileptr = ft.fileptr + nbytes;
                nbytes = 0;
                return original_nbytes;
            }
        }
    }
    bytes = original_nbytes - nbytes;
    return bytes;
}

int fs_write(int fd, void *buf, int nbytes) {
  int i, bytes, original_nbytes = nbytes;
    struct filetable ft = oft[fd];
    struct inode node = ft.in;
    int inode_offset = (ft.fileptr % fsd.blocksz);
    int inodeblock = (ft.fileptr / fsd.blocksz);
    int new_blocks = fsd.blocksz - inode_offset;

    if(oft[fd].state == FSTATE_CLOSED){
        printf("File is in closed state\n");
        return SYSERR;
    }
    else if(nbytes == 0){
        printf("File is empty\n");
        return SYSERR;
    }
    else if(fd < 0 || fd > NUM_FD){
        printf("Invalid File\n");
        return SYSERR;
    }
    else if(inodeblock < INODEBLOCKS){
        while(nbytes > 0){
            if(node.blocks[inodeblock] > 0){
                i = node.blocks[inodeblock];
            }
            else if(node.blocks[inodeblock] == 0){
                int blocks, j;
                blocks = fsd.nblocks;
                for(j = 0; j < blocks; j++){
                    if(fs_getmaskbit(j) == 0){
                        i = j;
                    }
                }
                memcpy(node.blocks + inodeblock, &i, sizeof(int));
                memcpy(&((oft + fd)->in), &(node), sizeof(struct inode));
                ft.in = node;
                fs_put_inode_by_num(0, node.id, &node);
                fs_setmaskbit(i);
            }
            if(new_blocks > nbytes){
                bs_bwrite(0, i, inode_offset, buf, nbytes);
                ft.fileptr = ft.fileptr + nbytes;
                nbytes = 0;
                memcpy(oft + fd, &ft, sizeof(struct filetable));
                return original_nbytes;
            }
            if(new_blocks <= nbytes){
                if(inodeblock == INODEBLOCKS - 1){
                    printf("Byte limit exceeded\n");
                    bytes = original_nbytes - nbytes;
                    return bytes;
                }
                bs_bwrite(0, i, inode_offset, buf, new_blocks);
                buf = buf + new_blocks;
                nbytes = nbytes - new_blocks;
                ft.fileptr = ft.fileptr + new_blocks;
                memcpy(oft + fd, &ft, sizeof(struct filetable));
                inodeblock++;
                inode_offset = 0;
            }
        }
    }
    bytes = original_nbytes - nbytes;
    return bytes;
}

int fs_link(char *src_filename, char* dst_filename) {
  int fd;
    struct inode node;
    struct filetable ft;
    for (fd = 0; fd < fsd.root_dir.numentries; fd++){       
        if(strncmp(src_filename, fsd.root_dir.entry[fd].name, FILENAMELEN + 1) == 0){
            ft.de = &(fsd.root_dir.entry[fsd.root_dir.numentries++]);
            (ft.de)->inode_num = fsd.root_dir.entry[fd].inode_num;         
            strcpy((ft.de)->name, dst_filename);
            fs_get_inode_by_num(0, fsd.root_dir.entry[fd].inode_num, &node);
            node.nlink = node.nlink + 1;
            fs_put_inode_by_num(0, fsd.root_dir.entry[fd].inode_num, &node);
            return OK;
        }
    }
    return SYSERR;
}

int fs_unlink(char *filename) {
  int fd, i;
    struct inode node;
    for(fd = 0; fd < fsd.root_dir.numentries; fd++){
        if(strncmp(filename, fsd.root_dir.entry[fd].name, FILENAMELEN + 1) == 0){   
            fs_get_inode_by_num(0, fsd.root_dir.entry[fd].inode_num, &node);
            if (node.nlink > 1){
                node.nlink -= 1;
                fs_put_inode_by_num(0, fsd.root_dir.entry[fd].inode_num, &node);
                fsd.root_dir.numentries--;
                return OK;
            }
            else if(node.nlink == 1){
                int blkLength = sizeof(node.blocks) / sizeof(node.blocks[0]);
                for (i = 0; i < blkLength; i++){
                    fs_clearmaskbit(i);
                }
                fs_put_inode_by_num(0, fsd.root_dir.entry[fd].inode_num, &node);
                fsd.root_dir.numentries--;
                return OK;
            }
        }
    }

    return SYSERR;
}
#endif /* FS */