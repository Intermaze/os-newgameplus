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
    initialize_filesystem_ext2();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);

    int row = 0, col = 0;
    keyboard_state_activate();


    char buffer[20] = "ANJING KOK SUSAH";
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

        /**
         * Idk it bootloop after writing to the disk 
         */
    if(retval == 0){
        framebuffer_write(row, col, 0x61, 0xF, 0);
    } else {
        framebuffer_write(row, col, 0x62, 0xF, 0);
        if (retval == 1) framebuffer_write(row, 1, 0x63, 0xF, 0); //file or folder already exist
    }
    
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



