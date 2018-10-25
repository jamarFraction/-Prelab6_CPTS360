#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "ext2fs/ext2_fs.h"

#define BLKSIZE 1024

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

int get_block(int fd, int blk, char buf[ ]);
int tst_bit(char *buf, int bit);
int put_block(int fd, int blk, char buf[ ]);
int super();
int gd();
int Imap();
int ialloc(int dev);
int set_bit(char *buf, int bit);

GD    *gp;
SUPER *sp;
INODE *ip;
DIR *dp;
char buf[BLKSIZE];
int fd;
int iblock;
char *disk = "mydisk";
int imap, bmap;  // IMAP and BMAP block number
int ninodes, nblocks, nfreeInodes, nfreeBlocks;
int search(INODE *ip, char *name);
int clr_bit(char *buf, int bit);
int decFreeInodes(int dev);

int main(int argc, char *argv[])
{
    if (argc > 1)
        disk = argv[1];
    fd = open(disk, O_RDONLY);
    if (fd < 0)
    {
        printf("open failed\n");
        exit(1);
    }

    //For the super block
    //super();

    //Group Descriptor
    //gd();

    //Inode Bitmap
    //Imap();

    //Blocks Bitmap
    //bbmap();

    //KC's potentially problematic function
    //inode();

    //My equally as probematic attempt
    //dir();

    //start for ialloc/balloc
    int i, ino, block;
    char buf[BLKSIZE];

    if (argc > 1)
        disk = argv[1];

    fd = open(disk, O_RDWR);
    if (fd < 0)
    {
        printf("open %s failed\n", disk);
        exit(1);
    }

    // read SUPER block
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;

    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;
    nfreeInodes = sp->s_free_inodes_count;
    nfreeBlocks = sp->s_free_blocks_count;
    printf("ninodes=%d nblocks=%d nfreeInodes=%d nfreeBlocks=%d\n",
           ninodes, nblocks, nfreeInodes, nfreeBlocks);

    // read Group Descriptor 0
    get_block(fd, 2, buf);
    gp = (GD *)buf;

    imap = gp->bg_inode_bitmap;
    bmap = gp->bg_block_bitmap;

    printf("imap = %d\n", imap);
    printf("bmap = %d\n", bmap);

    getchar();

    for (i = 0; i < 5; i++)
    {
        ino = ialloc(fd);

        printf("allocated ino = %d\n", ino);
    }
    
    for (i = 0; i < 5; i++)
    {
        block = balloc(fd);

        printf("allocated block = %d\n", block);
    }
}

int balloc(int dev)
{

    int i;
    char buf[BLKSIZE];

    // read inode_bitmap block
    get_block(dev, bmap, buf);

    for (i = 0; i < ninodes; i++)
    {
        if (tst_bit(buf, i) == 0)
        {
            set_bit(buf, i);
            decFreeInodes(dev);

            put_block(dev, bmap, buf);

            return i + 1;
        }
    }
    printf("balloc(): no more free blocks\n");
    return 0;
}

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

int super()
{
  // read SUPER block
  get_block(fd, 1, buf);  
  sp = (SUPER *)buf;

  // check for EXT2 magic number:
  printf("s_magic = %x\n", sp->s_magic);
  if (sp->s_magic != 0xEF53){
    printf("NOT an EXT2 FS\n");
    exit(1);
  }

  //Print information on the SUPER block  
  printf("EXT2 FS OK\n");
  printf("s_inodes_count = %d\n", sp->s_inodes_count);
  printf("s_blocks_count = %d\n", sp->s_blocks_count);
  printf("s_free_inodes_count = %d\n", sp->s_free_inodes_count);
  printf("s_free_blocks_count = %d\n", sp->s_free_blocks_count);
  printf("s_first_data_blcok = %d\n", sp->s_first_data_block);
  printf("s_log_block_size = %d\n", sp->s_log_block_size);
  printf("s_blocks_per_group = %d\n", sp->s_blocks_per_group);
  printf("s_inodes_per_group = %d\n", sp->s_inodes_per_group);
  printf("s_mnt_count = %d\n", sp->s_mnt_count);
  printf("s_max_mnt_count = %d\n", sp->s_max_mnt_count);
  printf("s_magic = %x\n", sp->s_magic);
  printf("s_mtime = %s\n", ctime(&sp->s_mtime));
  printf("s_wtime = %s\n", ctime(&sp->s_wtime));
}

int gd(){

    //Get and set for the Group Descriptor (there is only one for a floppy!)
    get_block(fd, 2, buf);
    gp = (GD *)buf;

    printf("Group Descriptor Information:\n");
    printf("bg_block_bitmap = %d\n", gp->bg_block_bitmap);
    printf("bg_inode_bitmap = %d\n", gp->bg_inode_bitmap);
    printf("bg_inode_table = %d\n", gp->bg_inode_table);
    printf("bg_free_blocks_count = %d\n", gp->bg_free_blocks_count);
    printf("bg_free_inodes_count = %d\n", gp->bg_free_inodes_count);
    printf("bg_used_dirs_count = %d\n", gp->bg_used_dirs_count);
    

}

int Imap()
{
    char buf[BLKSIZE];
    int imap, ninodes;
    int i;

    // read SUPER block
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;

    ninodes = sp->s_inodes_count;
    printf("ninodes = %d\n", ninodes);

    // read Group Descriptor
    get_block(fd, 2, buf);
    gp = (GD *)buf;

    imap = gp->bg_inode_bitmap;
    printf("Inodes bmap starts at: %d\n", imap);

    // read inode_bitmap block
    get_block(fd, imap, buf);

    for (i = 0; i < ninodes; i++)
    {
        (tst_bit(buf, i)) ? putchar('1') : putchar('0');

        if (i && (i % 8) == 0)
            printf(" ");
    }
    printf("\n");
}

int tst_bit(char *buf, int bit)
{
    int i, j;
    i = bit / 8;
    j = bit % 8;
    if (buf[i] & (1 << j))
        return 1;
    return 0;
}

int bbmap()
{

    char buf[BLKSIZE];
    int bbmap, nblocks;
    int i;

    // read SUPER block
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;

    nblocks = sp->s_blocks_count;
    printf("nblocks = %d\n", nblocks);

    // read Group Descriptor
    get_block(fd, 2, buf);
    gp = (GD *)buf;

    bbmap = gp->bg_block_bitmap;
    printf("block bmap starts at: %d\n", bbmap);

    // read blocks_bitmap block
    get_block(fd, bbmap, buf);

    for (i = 0; i < nblocks; i++)
    {
        (tst_bit(buf, i)) ? putchar('1') : putchar('0');

        if (i && (i % 8) == 0)
            printf(" ");
    }
    printf("\n");
}

int inode()
{
  char buf[BLKSIZE];

  // read GD
  get_block(fd, 2, buf);
  gp = (GD *)buf;
  
  iblock = gp->bg_inode_table;   // get inode start block#
  printf("inode_block=%d\n", iblock);

  // get inode start block     
  get_block(fd, iblock, buf);

  ip = (INODE *)buf + 1;         // ip points at 2nd INODE
  
  printf("mode=0x%4x\n", ip->i_mode);
  printf("uid=%d  gid=%d\n", ip->i_uid, ip->i_gid);
  printf("size=%d\n", ip->i_size);
  printf("time=%s", ctime(&ip->i_ctime));
  printf("link=%d\n", ip->i_links_count);
  printf("i_block[0]=%d\n", ip->i_block[0]);
  
}

int dir(){

    char *cp;
	int iblock;

	get_block(fd, 2, buf);
	gp = (GD *)buf;

	iblock = gp->bg_inode_table;
	printf("inode block %d\n", iblock);

	get_block(fd, iblock, buf);
	ip = (INODE *)buf + 1;

	get_block(fd, ip->i_block[0], buf);

    //Per KC instructions
	dp = (DIR *)buf;
    cp = (DIR *)buf;

    //Iterate over each entry in the directory
	while(dp->rec_len != 0)
	{
		printf("inode %d\n", dp->inode);
		printf("name length: %d\n%s\n", dp->name_len, dp->name);
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}
}

int search(INODE *ip, char *name){

    char *cp;
    char curName[EXT2_NAME_LEN];

    get_block(fd, ip->i_block[0], buf);

    //Per KCW instructions
	dp = (DIR *)buf;
    cp = (DIR *)buf;

    //Iterate over each entry in the directory
	while(dp->rec_len != 0)
	{
        strcpy(curName, dp->name);

        if(strcmp(curName, name) == 0){
            return 1;
        }
		
        //Word to KCW
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}

    //Target was not found
    return 0;

}

int put_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  write(fd, buf, BLKSIZE);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

int decFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}


int ialloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, imap, buf);

       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}