#ifndef _EXT2_H
#define _EXT2_H

#include "disk.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


/* -- IF2230 File System constants -- */
#define BOOT_SECTOR 0 // legacy from FAT32 filesystem IF2230 OS
#define DISK_SPACE 4194304u // 4MB disk space (because our disk or storage.bin is 4MB)
#define EXT2_SUPER_MAGIC 0xEF53 // this indicating that the filesystem used by OS is ext2
#define INODE_SIZE sizeof(struct EXT2INode) // size of inode
#define INODES_PER_TABLE (BLOCK_SIZE / INODE_SIZE) // number of inode per block (512 / )
#define GROUPS_COUNT (BLOCK_SIZE / sizeof(struct EXT2BlockGroupDescriptor)) / 2u // number of groups in the filesystem
#define BLOCKS_PER_GROUP (DISK_SPACE / GROUPS_COUNT / BLOCK_SIZE) // number of blocks per group
#define INODES_TABLE_BLOCK_COUNT 16u 
#define INODES_PER_GROUP (INODES_PER_TABLE * INODES_TABLE_BLOCK_COUNT) // number of inodes per group



/**
 * inodes constant 
 * - reference: https://www.nongnu.org/ext2-doc/ext2.html#inode-table
 */
#define EXT2_S_IFREG 0x8000 // regular file 
#define EXT2_S_IFDIR 0x4000 // directory


/* FILE TYPE CONSTANT*/
/**
 * reference: 
 * - https://www.nongnu.org/ext2-doc/ext2.html#linked-directories
 * - Table 4.2. Defined Inode File Type Values
 */

// #define EXT2_FT_UNKNOWN 0 // Unknown File Type
#define EXT2_FT_REG_FILE 1 // Regular File
#define EXT2_FT_DIR 2 // Directory
#define EXT2_FT_NEXT 3 // Next indode (?)
// #define EXT2_FT_CHRDEV 3 // Character Device [NOT USED]
// #define EXT2_FT_BLKDEV 4 // Block Device [NOT USED]
// #define EXT2_FT_FIFO 5 // Buffer file [NOT USED]
// #define EXT2_FT_SOCK 6 // Socket File [NOT USED]
// #define EXT2_FT_SYMLINK 7 // Symbolic Link File [NOT USED]


/**
 * EXT2DriverRequest
 * Derived dand modified from FAT32DriverRequest legacy IF2230 OS 
 */
struct EXT2DriverRequest
{
    void *buf; 
    char *name; 
    uint8_t name_len; 
    uint32_t inode; 
    uint32_t buffer_size; 

    bool is_directory; // for delete
    bool is_inode; // to get the directory inode without loading the buffers
}__attribute__((packed));

/**
 * EXT2Superblock: 
 * - https://www.nongnu.org/ext2-doc/ext2.html#superblock
 */
struct EXT2Superblock
{
    uint32_t inodes_count;        // 32bit value indicating the total number of inodes, both used and free, in the file system 
    uint32_t blocks_count;        // 32bit value indicating the total number of blocks in the system including all used, free and reserved 

    uint32_t r_blocks_count;      // 32bit value indicating the total number of blocks reserved for the usage of the super user. {maybe not used because there is no superuser in our system} 
    uint32_t free_blocks_count;   // 32bit value indicating the total number of free blocks, including the number of reserved blocks 
    uint32_t free_inodes_count;   // 32bit value indicating the total number of free inodes. This is a sum of all free inodes of all the block groups.
    uint32_t first_data_block;    // 32bit value identifying the first data block, in other word the id of the block containing the superblock structure.

    uint32_t blocks_per_group;    
    /** 32bit value indicating the total number of blocks per group. 
     *  This value in combination with s_first_data_block can be used to determine the block groups boundaries. 
     *  Due to volume size boundaries, the last block group might have a smaller number of blocks than what is specified in this field. */

    uint32_t frags_per_group; 
    /**
     * 32bit value indicating the total number of fragments per group. It is also used to determine the size of the block bitmap of each block group.
     */

    uint32_t inodes_per_group; 
    /**
     * 32bit value indicating the total number of inodes per group. This is also used to determine the size of the inode bitmap of each block group. 
     * Note that you cannot have more than (block size in bytes * 8) inodes per group as the inode bitmap must fit within a single block. 
     * This value must be a perfect multiple of the number of inodes that can fit in a block ((1024<<s_log_block_size)/s_inode_size).
     */

    uint16_t magic; // 16bit value indicating the file system type. For ext2, this value is 0xEF53.(DEFINE as EXT2_SUPER_MAGIC)
    uint32_t first_ino; // 32bit value used as index to the first inode useable for standard files. In revision 0, the first non-reserved inode is fixed to 11 (EXT2_GOOD_OLD_FIRST_INO). In revision 1 and later this value may be set to any value.

    uint8_t prealloc_blocks; // 8bit value indicating the number of blocks to preallocate for files.
    uint8_t prealloc_dir_blocks; // 8bit value indicating the number of blocks to preallocate for directories.


}__attribute__((packed));


/**
 * reference: 
 * - https://www.nongnu.org/ext2-doc/ext2.html#block-group-descriptor-table
 */
struct EXT2BlockGroupDescriptor
{
    /**
     * 32bit block id of the first block of the “block bitmap” for the group represented.
     * The actual block bitmap is located within its own allocated blocks starting at the block ID specified by this value.    
     */
    uint32_t block_bitmap; 

    /**
     * 32bit block id of the first block of the “inode bitmap” for the group represented.
     */
    uint32_t inode_bitmap;

    /**
     * 32bit block id of the first block of the “inode table” for the group represented.
     */
    uint32_t inode_table;       

    /**
     * 16bit value indicating the total number of free blocks for the represented group.
     */
    uint16_t free_blocks_count;

    /**
     * 16bit value indicating the total number of free inodes for the represented group.
     */
    uint16_t free_inodes_count;

    /**
     * 16bit value indicating the number of inodes allocated to directories for the represented group.
     */
    uint16_t used_dirs_count;

    /**
     * 16bit value used for padding the structure on a 32bit boundary.
     */
    uint16_t pad;

    /**
     * 12 bytes of reserved space for future revisions.
     */
    uint32_t reserved[3]; // 12 bytes of reserved space for future revisions. 
}__attribute__((packed));

/**
 * reference: 
 * - https://www.nongnu.org/ext2-doc/ext2.html#block-group-descriptor-table
 */
struct EXT2BlockGroupDescriptorTable
{
    struct EXT2BlockGroupDescriptor table[GROUPS_COUNT]; // can be change with fixed size array
};


/**
 * EXT2Inode
 * Inode stands for index node, it is a data structure in a Unix-style file system that describes a file-system object such as a file or a directory.
 */

struct EXT2INode
{
    uint16_t mode; // 16bit value used to indicate the format of the described file and the access rights. (for now, for file or directory)
    uint16_t uid; // 16bit value indicating the user ID of the file owner.
    uint32_t size_low; // ("i_size" in documentation) 32bit value indicating the size of the file in bytes. (lower 32 bit for filesize, higher 32 bit in i_dir_acl. Not using higher bit means file max size = 2^32-1 aka 4GB)
    uint32_t atime; // 32bit value indicating the time the file was last accessed.
    uint32_t ctime; // 32bit value indicating the time the file was created.
    uint32_t mtime; // 32bit value indicating the time the file was last modified.
    uint32_t dtime; // 32bit value representing the number of seconds since january 1st 1970, of when the inode was deleted. 

    uint16_t gid; // 16bit value indicating the group ID of the file owner. [NOT USED]
    uint16_t links_count; // 16bit value indicating the number of hard links to the file.  [NOT USED]
    uint32_t blocks; // 32bit value indicating the number of blocks used by the file.
    uint32_t flags; // 32bit value indicating the file flags. [NOT USED]

    /**
     * 15 x 32bit block numbers pointing to the blocks containing the data for this inode
     * 
     * - The first 12 blocks are direct blocks
     * - The 13th entry in this array is the block number of the first indirect block which is a block containing an array of block ID containing the data
     * Therefore, the 13th block of the file will be the first block ID contained in the indirect block. With a 1KiB block size, blocks 13 to 268 of the file data are contained in this indirect block.
     * - The 14th entry in this array is the block number of the first doubly-indirect block
     * - The 15th entry in this array is the block number of the triply-indirect block
     * 
     * maybe this video will help
     * - https://www.youtube.com/watch?v=tMVj22EWg6A
     *  
     */
    uint32_t block[15];

    uint32_t generation; // 32bit value indicating the file version (used by NFS). [NOT USED]
    uint32_t file_acl; // 32bit value indicating the block number containing the extended attributes. In revision 0 this value is always 0 [NOT USED]
    uint32_t size_high; // ("i_dir_acl" in documentation) In revision 0 this 32bit value is always 0. In revision 1, for regular files this 32bit value contains the high 32 bits of the 64bit file size. [NOT USED]

    uint32_t faddr; // 32bit value indicating the fragment address. 

    uint32_t osd1[3]; // 12 bytes of OS dependent data. [don't know what is this]


}__attribute__((packed));

struct EXT2INodeTable
{
    struct EXT2INode table[INODES_PER_GROUP]; // can be change with fixed size array
};

/**
 * EXT2DirectoryEntry
 * Linked List Directory
 * reference: 
 * - https://www.nongnu.org/ext2-doc/ext2.html#linked-directories
 */

struct EXT2DirectoryEntry
{
    uint32_t inode; // 32bit value indicating the inode number of the file entry. A value of 0 indicate that the entry is not used.
    /**
     * 16bit unsigned displacement to the next directory entry from the start of the current directory entry.
     * This field must have a value at least equal to the length of the current record.
     * The directory entries must be aligned on 4 bytes boundaries and there cannot be any directory entry spanning multiple data blocks.
     * If an entry cannot completely fit in one block, it must be pushed to the next data block and the rec_len of the previous entry properly adjusted.
     */
    uint16_t rec_len; 

    /**
     * 8bit value indicating the length of the file name.
     */
    uint8_t name_len;

    /**
     * 8bit unsigned value used to indicate file type.
     */
    uint8_t file_type;

}__attribute__((packed));

/**
 *  REGULAR function
 */

 /**
 * @brief get name of a directory entry, because name is dynamic, its available after the struct
 * @param entry pointer of the directory entry
 * @return entry name with length of entry->name_len
 */
char *get_entry_name(void *entry);

/**
 * @brief get directory entry from directory table, because
 * its list with dynamic size of each item, offset
 * (byte location) is used instead of index
 * @param ptr pointer of directory table
 * @param offset entry offset from ptr
 * @return pointer of directory entry with such offset
 */
struct EXT2DirectoryEntry *get_directory_entry(void *ptr, uint32_t offset);

/**
 * @brief get next entry of a directory entry, it is located
 * after the entry rec len
 * @param entry pointer of entry
 * @returns next entry
 */
struct EXT2DirectoryEntry *get_next_directory_entry(struct EXT2DirectoryEntry *entry);

/**
 * @brief get record length of a directory entry
 * that has dynamic size based on its name length, struct size is 12
 * and after that the buffer will contain its name char * that needs to aligned at 4 bytes boundaries
 * @param name_len entry name length (includes null terminator)
 * @returns sizeof(EXT2DirectoryEntry) + name_len aligned 4 bytes
 */
uint16_t get_entry_record_len(uint8_t name_len);

/**
 * @brief get first directory entry child offset
 * @return offset from the directory table
 */
uint32_t get_dir_first_child_offset(void *ptr);


/* =================== MAIN FUNCTION OF EXT32 FILESYSTEM ============================*/

/**
 * @brief get bgd index from inode, inode will starts at index 1
 * @param inode 1 to INODES_PER_GROUP * GROUP_COUNT
 * @return bgd index (0 to GROUP_COUNT - 1)
 */
uint32_t inode_to_bgd(uint32_t inode);

/**
 * @brief get inode local index in the corrresponding bgd
 * @param inode 1 to INODES_PER_GROUP * GROUP_COUNT
 * @return local index
 */
uint32_t inode_to_local(uint32_t inode);

/**
 * @brief create a new directory using given node
 * first item of directory table is its node location (name will be .)
 * second item of directory is its parent location (name will be ..)
 * @param node pointer of inode
 * @param inode inode that already allocated
 * @param parent_inode inode of parent directory (if root directory, the parent is itself)
 */
void init_directory_table(struct EXT2INode *node, uint32_t inode, uint32_t parent_inode);
/**
 * @brief check whether filesystem signature is missing or not in boot sector
 *
 * @return true if memcmp(boot_sector, fs_signature) returning inequality
 */
bool is_empty_storage(void);

/**
 * @brief create a new EXT2 filesystem. Will write fs_signature into boot sector,
 * initialize super block, bgd table, block and inode bitmap, and create root directory
 */
void create_ext2(void);

/**
 * @brief Initialize file system driver state, if is_empty_storage() then create_ext2()
 * Else, read and cache super block (located at block 1) and bgd table (located at block 2) into state
 */
void initialize_filesystem_ext2(void);

/**
 * @brief check whether a directory table has children or not
 * @param inode of a directory table
 * @return true if first_child_entry->inode = 0
 */
bool is_directory_empty(uint32_t inode);



/* =============================== CRUD FUNC ======================================== */

int8_t read_dir(struct EXT2DriverRequest *request);

int8_t read(struct EXT2DriverRequest request);

int8_t read_next_dir_table(struct EXT2DriverRequest request);

int8_t write(struct EXT2DriverRequest *request);

int8_t delete_entry(struct EXT2DriverRequest request);

// int8_t move_dir(struct EXT2DriverRequest request_src, struct EXT2DriverRequest dst_request);

/* =============================== MEMORY ==========================================*/

uint32_t allocate_node(void); 

void deallocate_node(uint32_t inode);

void deallocate_blocks(void *loc, uint32_t blocks);

uint32_t deallocate_block(uint32_t *locations, uint32_t blocks, struct BlockBuffer *bitmap, uint32_t depth, uint32_t *last_bgd, bool bgd_loaded);

void allocate_node_blocks(void *ptr, struct EXT2INode *node, uint32_t preferred_bgd);

void sync_node(struct EXT2INode *node, uint32_t inode);

/* ============================== UTILS ================================================ */

uint32_t map_node_blocks(void *ptr, uint32_t blocks, uint32_t *locations, uint32_t *mapped_count, uint8_t depth);

void search_blocks(uint32_t preferred_bgd, uint32_t *locations, uint32_t blocks, uint32_t *found_count);

void search_blocks_in_bgd(uint32_t bgd, uint32_t *locations, uint32_t blocks, uint32_t *found_count);

void load_inode_blocks(void *ptr, void *_block, uint32_t size);

uint32_t load_blocks_rec(void *ptr, uint32_t block, uint32_t block_size, uint32_t size, uint8_t depth);

bool is_same_dir_entry(struct EXT2DirectoryEntry *entry, struct EXT2DriverRequest request, bool is_file);

#endif