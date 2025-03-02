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
    return (struct EXT2DirectoryEntry *)(ptr + offset);
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
        return len;
    }
    return (cmp + 1) * 4;
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
    return (inode - 1) % INODES_PER_GROUP;
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
    
    if(bgd == -1) return 0;

    read_blocks(&block_buffer, bgd_table.table[bgd].bg_inode_bitmap, 1);
    uint32_t inode = bgd * INODES_PER_GROUP + 1;
    for(uint32_t i = 0; i < INODES_PER_GROUP; i++){
        if(!(block_buffer.buf[i / 8] & (1 << (i % 8)))){
            block_buffer.buf[i / 8] |= (1 << (i % 8));
            bgd_table.table[bgd].bg_free_inodes_count--;
            write_blocks(&block_buffer, bgd_table.table[bgd].bg_inode_bitmap, 1);
            return inode + i;
        }
    }
}

void deallocate_node(uint32_t inode){
    uint32_t bgd = inode_to_bgd(inode);
    read_blocks(&block_buffer, bgd_table.table[bgd].bg_inode_bitmap, 1);
    block_buffer.buf[inode_to_local(inode) / 8] &= ~(1 << (inode_to_local(inode) % 8));
    bgd_table.table[bgd].bg_free_inodes_count++;
    write_blocks(&block_buffer, bgd_table.table[bgd].bg_inode_bitmap, 1);
    write_blocks(&bgd_table, 2, 1);
}

void deallocate_blocks(void *loc, uint32_t blocks){
    if(blocks == 0) return;
    uint32_t last_bgd = -1;
    uint32_t deallocated;
    
    for(uint32_t i = 0; i < 15 && blocks > 0; i++){
        if(i < 12) deallocated = deallocate_block(loc, blocks, &block_buffer, 0, &last_bgd, false);
        else deallocated = deallocate_block(loc, blocks, &block_buffer, i - 11, &last_bgd, false);
        blocks -= deallocated;
    }

    write_blocks(&block_buffer, bgd_table.table[last_bgd].bg_block_bitmap, 1);
    write_blocks(&bgd_table, 2, 1);
}

uint32_t deallocate_block(uint32_t *locations, uint32_t blocks, struct BlockBuffer *bitmap, uint32_t depth, uint32_t *last_bgd, bool bgd_loaded){
    if(blocks == 0) return 0;
    uint32_t bgd = *locations / BLOCKS_PER_GROUP;
    if (!bgd_loaded || bgd != *last_bgd)
    {
        if (bgd_loaded)
        {
        // update previous bgd_bitmap
        write_blocks(bitmap, bgd_table.table[*last_bgd].bg_block_bitmap, 1);
        }
        // load a new one
        read_blocks(bitmap, bgd_table.table[bgd].bg_block_bitmap, 1);
        *last_bgd = bgd;
    }
    uint32_t local_idx = *locations % BLOCKS_PER_GROUP;
    uint8_t offset = 7 - local_idx % 8;
    // set flag of the block bitmap
    bitmap->buf[local_idx / 8] &= 0xFFu ^ (1u << offset);
    bgd_table.table[*last_bgd].bg_free_blocks_count++;
    if (depth == 0)
    {
        return 1;
    }
    struct BlockBuffer child_buf;

    // load the direct block
    read_blocks(child_buf.buf, *locations, 1);
    uint32_t *child_loc = (uint32_t *)child_buf.buf;
    uint32_t deallocated = 0;
    for (uint32_t i = 0; i < BLOCK_SIZE / 4u; i++)
    {
        uint32_t new_deallocated = deallocate_block(child_loc, blocks, bitmap, depth - 1, last_bgd, true);
        deallocated += new_deallocated;
        blocks -= new_deallocated;
        if (blocks == 0)
        {
        return deallocated;
        }
    }
    return deallocated;
}

void allocate_node_blocks(void *ptr, struct EXT2Inode *node, uint32_t preferred_bgd){
    for(uint32_t i = 0; i < 15; i++){
        if(i < 12){
            node->i_block[i] = allocate_node();
            write_blocks(ptr, node->i_block[i], 1);
        }
        else if(i == 12){
            node->i_block[i] = allocate_node();
            write_blocks(ptr, node->i_block[i], 1);
            struct BlockBuffer block;
            uint32_t *block_ptr = block.buf;
            for(uint32_t j = 0; j < BLOCK_SIZE / 4; j++){
                block_ptr[j] = allocate_node();
                write_blocks(ptr, block_ptr[j], 1);
            }
            write_blocks(&block, node->i_block[i], 1);
        }
    }
}

void sync_node(struct EXT2Inode *node, uint32_t inode){
    uint32_t bgd = inode_to_bgd(inode);
    uint32_t local = inode_to_local(inode);

    read_blocks(&inode_table_buf, bgd_table.table[bgd].bg_inode_bitmap, INODES_TABLE_BLOCK_COUNT);
    inode_table_buf.table[local] = *node;
    write_blocks(&inode_table_buf, bgd_table.table[bgd].bg_inode_bitmap, INODES_TABLE_BLOCK_COUNT);
}