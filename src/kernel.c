#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/cpu/interrupt.h"
#include "header/cpu/idt.h"

// void kernel_setup(void) {
//     load_gdt(&_gdt_gdtr);
//     while (true);
// } 

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    __asm__("int $0x4");
    while (true);
}


