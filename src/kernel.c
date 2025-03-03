#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/cpu/interrupt.h"
#include "header/cpu/idt.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/disk.h"
#include "header/filesystem/ext2.h"

// void kernel_setup(void) {
//     load_gdt(&_gdt_gdtr);
//     while (true);
// } 

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);

    create_ext2();    
   
    int row = 0, col = 0;
    keyboard_state_activate();

    initialize_filesystem_ext2();
    char buffer[20] = "JANCOK";
    struct EXT2DriverRequest req =
    {
        .buf = buffer,
        .name = "halo",
        .inode = 1,
        .buffer_size = 20,
        .name_len = 4,
    };
    int8_t retval;
    retval = write(&req);
    (void)retval;
    
    // struct BlockBuffer b;
    // for (int i = 0; i < 512; i++) b.buf[i] = 0x61;
    // write_blocks(&b, 17, 1);
    while (true) {
        char c;
        get_keyboard_buffer(&c);
        if (c) {
            framebuffer_write(row, col, c, 0xF, 0);
            if (col >= BUFFER_WIDTH) {
                ++row;
                col = 0;
            } else {
                ++col;
            }
           framebuffer_set_cursor(row, col);
        }
    }
}



