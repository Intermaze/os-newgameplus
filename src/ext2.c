#include "header/filesystem/ext2.h"

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
    '3',
    '\n',
    [BLOCK_SIZE - 2] = 'O',
    [BLOCK_SIZE - 1] = 'k',
};


struct EXT2Superblock sblock = {}; 
struct BlockBuffer block_buffer = {}; 
struct EXT2BlockGroupDescriptorTable bgd_table = {};
struct EXT2INodeTable inode_table_buf = {};

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
    return (inode - 1) % INODES_PER_GROUP;
}

void init_directory_table(struct EXT2INode *node, uint32_t inode, uint32_t parent_inode){
    struct BlockBuffer block;

    struct EXT2DirectoryEntry *table = get_directory_entry(&block, 0); // .
    table->inode = inode;
    table->file_type = EXT2_FT_DIR;
    table->name_len = 1;
    memcpy(get_entry_name(table), ".", 2); // !remindme to include header for memcpy
    table->rec_len = get_entry_record_len(table->name_len); // to add "padding" to the dynamic name size to mod4 for optimization
  
    struct EXT2DirectoryEntry *parent_table = get_next_directory_entry(table); // ..
    parent_table->inode = parent_inode;
    parent_table->file_type = EXT2_FT_DIR;
    parent_table->name_len = 2;
    memcpy(get_entry_name(parent_table), "..", 3);
    parent_table->rec_len = get_entry_record_len(parent_table->name_len); 
  
    struct EXT2DirectoryEntry *first_file = get_next_directory_entry(parent_table); // initialize inode for first file
    first_file->inode = 0;
  
    node->mode = EXT2_S_IFDIR; // this is a directory
    node->size_low = 0;
    node->size_high = 0;
  
    // get_timestamp(); implement after shell?
    // node->mtime = node->ctime = node->atime = get_timestamp();
    // node->dtime = 0;
  
    node->blocks = 1;
    allocate_node_blocks(&block, node, inode_to_bgd(inode));
  
    sync_node(node, inode);
}

bool is_empty_storage(void){
    read_blocks(&block_buffer, BOOT_SECTOR, 1);
    return memcmp(&block_buffer, fs_signature, BLOCK_SIZE);
}

void create_ext2(void);

void initialize_filesystem_ext2(void){
    if (is_empty_storage())
    {
      create_ext2();
    }
    else
    {
      read_blocks(&sblock, 1, 1);
      read_blocks(&bgd_table, 2, 1);
    }
}

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








