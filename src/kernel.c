#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    while (true);
}
