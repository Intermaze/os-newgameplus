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

    node->i_mode = EXT2_S_IFDIR;
    node->i_blocks = 1;
    
    allocate_node_blocks(&block, node);
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
    sblock.s_free_blocks_count = sblock.s_blocks_count;
    sblock.s_first_data_block = 1;
    sblock.s_blocks_per_group = BLOCKS_PER_GROUP;
    sblock.s_inodes_per_group = INODES_PER_GROUP;
    sblock.s_magic = EXT2_SUPER_MAGIC;
    sblock.s_first_ino = 1;
    write_blocks(&sblock, 1, 1);

    struct BlockBuffer inode_bitmap;
    struct BlockBuffer block_bitmap;

    memset(&inode_bitmap, 0, BLOCK_SIZE);
    memset(&block_bitmap, 0, BLOCK_SIZE);

    for (uint32_t i = 0; i < GROUPS_COUNT; i++){
        uint32_t reserved_blocks = 2u + INODES_TABLE_BLOCK_COUNT; // block_bitmap, inode_bitmap, inodes_table
        if (i == 0)
        {
        // 3 first block is for boot sector, superblock, and bgd table
        reserved_blocks += 3;
        bgd_table.table[i].bg_block_bitmap = 3;

        // set block_bitmap for first block
        uint32_t j = 0;
        uint32_t k = 0;
        while (j * 8 < reserved_blocks)
        {
            k = reserved_blocks - (j * 8);
            if (k > 8)
            k = 8;

            // set first k bit of a byte to 1 and rest to 0
            block_bitmap.buf[j] = 0xFFu - ((1u << (8u - k)) - 1u);
            j++;
        }

        // make j and k actually represent last byte and bit
        j--;
        k--;
        write_blocks(&block_bitmap, bgd_table.table[i].bg_block_bitmap, 1);

        // set back the last 3 bitmap_block bit to 0
        for (uint8_t l = 0; l < 3; l++)
        {
            uint8_t offset = 7u - k;
            block_bitmap.buf[j] &= 0xFFu - (1u << offset);
            if (k == 0)
            {
            j--;
            k = 7;
            }
            else
            k--;
        }
        }
        else
        {
        bgd_table.table[i].bg_block_bitmap = i * sblock.s_blocks_per_group + 0;
        write_blocks(&block_bitmap, bgd_table.table[i].bg_block_bitmap, 1);
        }
        bgd_table.table[i].bg_free_blocks_count = sblock.s_blocks_per_group - reserved_blocks;
        bgd_table.table[i].bg_inode_bitmap = bgd_table.table[i].bg_block_bitmap + 1;

        // write inode bitmap
        write_blocks(&inode_bitmap, bgd_table.table[i].bg_inode_bitmap, 1);
        bgd_table.table[i].bg_inode_bitmap = bgd_table.table[i].bg_block_bitmap + 2;

        bgd_table.table[i].bg_free_inodes_count = sblock.s_inodes_per_group;

        bgd_table.table[i].bg_used_dirs_count = 0;
    }
    write_blocks(&bgd_table, 2, 1);
    struct EXT2Inode root_node;
    uint32_t root_inode = allocate_node();
    init_directory_table(&root_node, root_inode, root_inode);
}

void initialize_filesystem_ext2(void){
    if(is_empty_storage()) create_ext2();
    else{
        read_blocks(&sblock, 1, 1);
        read_blocks(&bgd_table, 2, 1);
    }
}

bool is_directory_empty(uint32_t inode){
    uint32_t bgd = inode_to_bgd(inode);
    uint32_t local = inode_to_local(inode);

    read_blocks(&block_buffer, bgd_table.table[bgd].bg_inode_bitmap, INODES_TABLE_BLOCK_COUNT);

    struct EXT2Inode *node = &inode_table_buf.table[local];
    struct BlockBuffer block;
    read_blocks(&block, node->i_block[0], 1);

    uint32_t offset = get_dir_first_child_offset(&block);
    struct EXT2DirectoryEntry *entry = get_directory_entry(&block, offset);
    return entry->inode == 0;
}

/* =============================== CRUD FUNC ======================================== */

int8_t read_directory(struct EXT2DriverRequest *prequest){
    struct EXT2DriverRequest request = *prequest;

    char dot[2] = "."; 

    /**
     * if the request name is "/", then it means that the request is for the root directory
     */
    if(!strcmp(request.name, "/", request.name_len)){
        request.inode = sblock.s_first_ino; 
        request.name = dot; 
        request.name_len = 1; 
    }

    int8_t retval = resolve_path(&request);
    if(retval != 0){
        return 3; // invalid parent folder, the number? idk check the guide book
    }

    /**
     * get the block group descriptor and the local index of the inode
     * and read the block
     */
    uint32_t bgd = inode_to_bgd(request.inode); 
    uint32_t local_idx = inode_to_local(request.inode); 
    read_blocks(&inode_table_buf, bgd_table.table[bgd].bg_inode_table, INODES_TABLE_BLOCK_COUNT);

    /**
     * collect the node from the inode table buffer
     */
    struct EXT2Inode *node = &inode_table_buf.table[local_idx];

    if(node->i_mode != EXT2_S_IFDIR){
        return 3; // invalid parent folder
    }

    //read the directory entry 
    struct BlockBuffer block_buffer; 
    read_blocks(&block_buffer, node->i_block[0], 1); 

    uint32_t offset = 0; 
    struct EXT2DirectoryEntry *entry = get_directory_entry(&block_buffer, offset);

    /**
     * iterate through the directory entry to get the requested entry
     * if entry inode == 0 the it mean the entry is empty
     */
    while(offset < BLOCK_SIZE && entry->inode != 0){
        if(entry->file_type == EXT2_FT_NEXT){
            read_blocks(&block_buffer, entry->inode, 1);
            offset = 0; 
            entry = get_directory_entry(&block_buffer, offset);
            continue;
        }

        if(is_same_dir_entry(entry, request, FALSE)){
            if(offset != 0){
                bgd = inode_to_bgd(entry->inode); 
                local_idx = inode_to_local(entry->inode); 
                read_blocks(&inode_table_buf, bgd_table.table[bgd].bg_inode_table, INODES_TABLE_BLOCK_COUNT); 
                node = &inode_table_buf.table[local_idx];
                if(!request.is_inode){
                    read_blocks(request.buf, node->i_block[0], 1); 
                }

            }
            else if(!request.is_inode){
                memcpy(request.buf, block_buffer.buf, BLOCK_SIZE);
            }
            prequest->inode = entry->inode; 

            sync_node(node, entry->inode);
            return 0;
        }

        offset += entry->rec_len; 
        entry = get_directory_entry(&block_buffer, offset); 
    }

    return 2;
}

int8_t read(struct EXT2DriverRequest request);

int8_t read_next_directpry_table(struct EXT2DriverRequest request);

int8_t write(struct EXT2DriverRequest *request){
    int8_t retval = resolve_path(request); 
    
    if(retval != 0){
        return 2; // invalid parent folder 
    }

    uint32_t bgd = inode_to_bgd(request->inode); 
    uint32_t local_idx = inode_to_local(request->inode); 

    read_blocks(&inode_table_buf, bgd_table.table[bgd].bg_inode_table, INODES_TABLE_BLOCK_COUNT);

    // choose the inode
    struct EXT2Inode *node = &inode_table_buf.table[local_idx];

    if(node->i_mode & EXT2_S_IFDIR){
        return 2; // not valid parent directory
    }

    // read the first block from node
    uint32_t block_num = node->i_block[0];


    //read the directory entry from the block 
    struct BlockBuffer block_buffer; 
    read_blocks(&block_buffer, block_num, 1);

    // getting the first child of the directory 
    /**
     * 1. first we need to get the offset of the first child of the directory
     * 2. then we can get the first child of the directory using the offset we already obtain 
     */
    uint32_t offset = get_dir_first_child_offset(&block_buffer); 
    struct EXT2DirectoryEntry *entry = get_directory_entry(&block_buffer, offset);

    /**
     * we need to get the length of the entry name 
     */
    uint16_t record_len = get_entry_record_len(request->name_len); 
    uint16_t space_total = BLOCK_SIZE - get_entry_record_len(0);

    bool isFound = FALSE; 

    // iterate through the directory entry to find the requested one 
    while(!isFound){
        if(entry->inode == 0){
            /**
             * if the inode is 0, then it means that the entry is empty
             */
            return 1; //honestly i forgor from where the number 1 comes from, maybe it's from the guide book, but meh i forgor 
        }

        if(entry->file_type == EXT2_FT_NEXT){
            /**
             * search through the next directory list that existed 
             * the step 
             * 
             * 1. get the next directory entry 
             * 2. read the next directory entry using read_block 
             * 3. get the offset of the next directory entry for the next iteration
             */
            block_num = entry->inode; 
            read_blocks(&block_buffer, block_num, 1);
            offset = 0; 
            entry = get_directory_entry(&block_buffer, offset); 
            continue;
        }

        if(entry->inode != 0){
            /**
             * this part is the part where we check whether the entry is the requested entry or not
             * because inode != 0 or it means that the entry is not empty
             * it will check both a file or a directory, it can determine from a boolean that passed as a parameter from the request 
             */
            
            if(is_same_dir_entry(entry, *request, TRUE) || is_same_dir_entry(entry, *request, FALSE)){
                // folder or a file is already exist 
                return 1;
            }
            // move to the next entry 
            /**
             * 1. the offset will be added by the record length of the entry 
             * 2. entry will update to the next directory entry from the current offset to the +offset 
             */
            offset += entry->rec_len; 
            entry = get_directory_entry(&block_buffer, offset);
            continue;
        }
        if(offset + record_len <= space_total){
            isFound = TRUE;
        }
        else{
            /**
             * if the offset + record length is greater than the space total, then it means that the entry is not found 
             * and the space is not enough to store the entry 
             * so we need to move to the next block until we find the empty space
             */
            uint32_t next_block; 
            uint32_t found_count = 0; 
            
            /**
             * search the block that has empty_space 
             */
            search_blocks(bgd, &next_block, 1, &found_count);

            if(found_count == 0){
                /**
                 * if the found count is 0, then it means that there is no empty space in the block
                 * it mean you are running out of space lol
                 */
                return -1; // check the guide book for the error code 
            }

            /**
             * if we found the block that has empty space, then we need to write the entry to the next_block
             */
            entry->inode = next_block;
            entry->name_len = 0; 
            entry->rec_len = get_entry_record_len(0);
            entry->file_type = EXT2_FT_NEXT;

            // write the entry to the block
            write_blocks(&block_buffer, block_num, 1);

            // load the new directory table after write_block
            read_blocks(&block_buffer, next_block, 1);
            isFound = TRUE;
            block_num = next_block; 
            offset = 0; 
            entry = get_directory_entry(block_buffer.buf, 0);
        }
    }

    /**
     * the actual writing of the entry to the block, bcs the previous part is just the searching of the empty space that
     * can be written
     */

    struct EXT2Inode new_node; // the new node that will be written to the block
    uint32_t new_inode = allocate_node(); // allocate the new inode for the new node
    entry->inode = new_inode; // set the inode of the entry to the new inode

    // set the entry name based on the request name and make the len + 1 because of the null terminator if there's any
    memcpy(get_entry_name(entry), request->name, request->name_len + 1); 
    entry->name_len = request->name_len;
    entry->rec_len = get_entry_record_len(entry->name_len); 

    /**
     * 
     */
    get_next_directory_entry(entry)->inode = 0; 

    if(request->buffer_size == 0){
        // if the buffer size is 0, then it means that the request is a directory
        entry->file_type = EXT2_FT_DIR;
        init_directory_table(&new_node, new_inode, request->inode);
        request->inode = new_inode;
    }
    else{
        // if the buffer size is not 0, then it means that the request is a file
        request->inode = new_inode; 
        entry->file_type = EXT2_FT_REG_FILE;
        new_node.i_mode = EXT2_S_IFREG;
        new_node.i_blocks = 1;
        new_node.i_size = request->buffer_size;

        new_node.i_blocks = 150;
        allocate_node_blocks(&block_buffer, &new_node);
        sync_node(&new_node, new_inode);
    }
    
    write_blocks(&block_buffer, block_num, 1);
    return 0; //success
}

int8_t delete(struct EXT2DriverRequest request);

int8_t move_dir(struct EXT2DriverRequest request_src, struct EXT2DriverRequest dst_request);

int8_t resolve_path(struct EXT2DriverRequest *request){
  if (request->name_len == 0) return 1;
  if (request->name[0] == '/'){
    // absolute path
    request->inode = sblock.s_first_ino;
    request->name++;
    request->name_len--;
    return resolve_path(request);
  }
  uint32_t len = 0;
  while (len < request->name_len && request->name[len] != '/'){
    len++;
  }
  if (len == request->name_len){
    // got the basic path
    return 0;
  }
   // read directory
   uint8_t prev_name_len = request->name_len;
   // bool prev_inode_only = request->inode_only;
   request->name_len = len;
   // request->inode_only = TRUE;
   int8_t retval = read_directory(request);
   if (retval != 0)
     return retval;
   // if abc/de, from name length 6 to 6 - 3 - 1 = 2 (de)
   request->name_len = prev_name_len - len - 1;
   request->name += len + 1;
   // request->inode_only = prev_inode_only;
   return resolve_path(request);
}

/* =============================== MEMORY ==========================================*/

uint32_t allocate_node(void){
    int32_t bgd = -1;
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
    return 0;
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

void allocate_node_blocks(void *ptr, struct EXT2Inode *node){
    for(uint32_t i = 0; i < 15; i++){
        if(i < 12){
            node->i_block[i] = allocate_node();
            write_blocks(ptr, node->i_block[i], 1);
        }
        else if(i == 12){
            node->i_block[i] = allocate_node();
            write_blocks(ptr, node->i_block[i], 1);
            struct BlockBuffer block;
            uint32_t *block_ptr = (uint32_t *) block.buf;
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

bool is_same_dir_entry(struct EXT2DirectoryEntry *entry, struct EXT2DriverRequest request, bool is_file){
    if (is_file && entry->file_type != EXT2_FT_REG_FILE)
    return FALSE;
  if (!is_file && entry->file_type != EXT2_FT_DIR)
    return FALSE;
  // name length must same
  if (request.name_len != entry->name_len)
    return FALSE;

  // entry name must same
  if (memcmp(request.name, get_entry_name(entry), request.name_len))
    return FALSE;

  return TRUE;
}

void search_blocks(uint32_t preferred_bgd, uint32_t *locations, uint32_t blocks, uint32_t *found_count)
{
    // Try searching in the preferred block group descriptor (BGD) first
    if (bgd_table.table[preferred_bgd].bg_free_blocks_count > 0)
    {
        search_blocks_in_bgd(preferred_bgd, locations, blocks, found_count);
    }

    // If not enough blocks found, search in other BGD groups
    for (uint32_t i = 0; i < GROUPS_COUNT && *found_count < blocks; i++)
    {
        if (i != preferred_bgd && bgd_table.table[i].bg_free_blocks_count > 0)
        {
            search_blocks_in_bgd(i, locations, blocks, found_count);
        }
    }
}

void search_blocks_in_bgd(uint32_t bgd, uint32_t *locations, uint32_t blocks, uint32_t *found_count)
{
    if (bgd_table.table[bgd].bg_free_blocks_count == 0)
        return;

    // Read the block bitmap
    read_blocks(&block_buffer, bgd_table.table[bgd].bg_block_bitmap, 1);
    uint32_t bgd_offset = bgd * BLOCKS_PER_GROUP;
    uint32_t allocated = 0;

    // Search for free blocks
    for (uint32_t i = 0; i < BLOCKS_PER_GROUP && *found_count < blocks && allocated < bgd_table.table[bgd].bg_free_blocks_count; i++)
    {
        uint8_t *byte = &block_buffer.buf[i / 8];
        uint8_t offset = 7 - (i % 8);

        if (!((*byte >> offset) & 1u))
        {
            // Mark the block as allocated
            *byte |= (1u << offset);
            locations[*found_count] = bgd_offset + i;
            (*found_count)++;
            allocated++;
        }
    }

    if (allocated > 0)
    {
        // Update block group descriptor and write changes
        bgd_table.table[bgd].bg_free_blocks_count -= allocated;
        write_blocks(&block_buffer, bgd_table.table[bgd].bg_block_bitmap, 1);
        write_blocks(&bgd_table, 2, 1);
    }
}