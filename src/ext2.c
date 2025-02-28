#include "header/filesystem/ext2.h"
#include "header/stdlib/string.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C',
    'o',
    'u',
    'r',
    's',
    'e',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    'D',
    'e',
    's',
    'i',
    'g',
    'n',
    'e',
    'd',
    ' ',
    'b',
    'y',
    ' ',
    ' ',
    ' ',
    ' ',
    ' ',
    'L',
    'a',
    'b',
    ' ',
    'S',
    'i',
    's',
    't',
    'e',
    'r',
    ' ',
    'I',
    'T',
    'B',
    ' ',
    ' ',
    'M',
    'a',
    'd',
    'e',
    ' ',
    'w',
    'i',
    't',
    'h',
    ' ',
    '<',
    '3',
    ' ',
    ' ',
    ' ',
    ' ',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '-',
    '2',
    '0',
    '2',
    '5',
    '\n',
    [BLOCK_SIZE - 2] = 'O',
    [BLOCK_SIZE - 1] = 'k',
};


struct EXT2Superblock sblock = {}; 
struct BlockBuffer block_buffer = {}; 
struct EXT2BlockGroupDescriptorTable bgd_table = {};
struct EXT2InodeTable inode_table_buf = {};

/* REGULAR FUNCTION */

char *get_entry_name(void *entry)
{
    return (char *)entry + sizeof(struct EXT2DirectoryEntry);
}

struct EXT2DirectoryEntry *get_directory_entry(void *ptr, uint32_t offset)
{
    return (struct EXT2DirectoryEntry *)((uint8_t *)ptr + offset);
}
struct EXT2DirectoryEntry *get_next_directory_entry(struct EXT2DirectoryEntry *entry)
{
    return get_directory_entry(entry, entry->rec_len);
}

uint16_t get_entry_record_len(uint8_t name_len)
{
    uint16_t len = sizeof(struct EXT2DirectoryEntry) + name_len + 1;

    uint32_t cmp = len / 4; 
    if (len % 4 == 0){
        return cmp;
    }
    return cmp + 1;
}

uint32_t get_dir_first_child_offset(void *ptr)
{
    uint32_t offset = 0;
    struct EXT2DirectoryEntry *entry = get_directory_entry(ptr, offset);
    offset += entry->rec_len;
    entry = get_next_directory_entry(entry);
    offset += entry->rec_len;
    return offset; 
}

/* ========================== MAIN FUNCTION ========================= */

uint32_t inode_to_bgd(uint32_t inode){
    return (inode - 1) / INODES_PER_GROUP;
}

uint32_t inode_to_local(uint32_t inode){
    return (inode -1) % INODES_PER_GROUP;
}

void init_directory_table(struct EXT2Inode *node, uint32_t inode, uint32_t parent_inode){
    struct BlockBuffer block;
    struct EXT2DirectoryEntry *entry = get_directory_entry(&block, 0);
    entry->inode = inode;
    entry->file_type = EXT2_FT_DIR;
    memcpy(get_entry_name(entry), ".", 2);
    entry->name_len = 1;
    entry->rec_len = get_entry_record_len(entry->name_len);

    struct EXT2DirectoryEntry *parent_entry = get_next_directory_entry(entry);
    parent_entry->inode = parent_inode;
    parent_entry->file_type = EXT2_FT_DIR;
    memcpy(get_entry_name(parent_entry), "..", 3);
    parent_entry->name_len = 2;
    parent_entry->rec_len = get_entry_record_len(parent_entry->name_len);

    struct EXT2DirectoryEntry *root_entry = get_next_directory_entry(parent_entry);
    root_entry->inode = 0;
    
    allocate_node_blocks(&block, node, inode_to_bgd(inode));
    sync_node(node, inode);
}

bool is_empty_storage(void){
    read_blocks(&block_buffer, BOOT_SECTOR, 1);
    return memcmp(&block_buffer, fs_signature, BLOCK_SIZE);
}

void create_ext2(void){
    write_blocks(fs_signature, BOOT_SECTOR, 1);
    sblock.s_inodes_count = GROUPS_COUNT * INODES_PER_GROUP;
    sblock.s_blocks_count = GROUPS_COUNT * BLOCKS_PER_GROUP;
    sblock.s_free_inodes_count =  sblock.s_inodes_count;
    // sblock.s_first_data_block ;

    // struct BlockBuffer block;
    // struct EXT2DirectoryEntry *entry = get_directory_entry(&block, 0);
}

/* =============================== MEMORY ==========================================*/

uint32_t allocate_node(void){
    uint32_t bgd = -1;
    for(uint32_t i = 0; i < GROUPS_COUNT; i++){
        if(bgd_table.table[i].bg_free_inodes_count > 0){
            bgd = i;
            break;
        }
    }
    
}

void deallocate_node(uint32_t inode);

void deallocate_blocks(void *loc, uint32_t blocks);

uint32_t deallocate_block(uint32_t *locations, uint32_t blocks, struct BlockBuffer *bitmap, uint32_t depth, uint32_t *last_bgd, bool bgd_loaded);

void allocate_node_blocks(void *ptr, struct EXT2Inode *node, uint32_t preferred_bgd);

void sync_node(struct EXT2Inode *node, uint32_t inode);