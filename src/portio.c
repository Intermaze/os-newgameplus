#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/cpu/portio.h"

/** x86 inb/outb:
 * @param dx target port 
 * @param al input/output byte
 */

void out(uint16_t port, uint8_t data) {
    __asm__(
        "outb %0, %1"
        : // <Empty output operand>
        : "a"(data), "Nd"(port)
    );
}

uint8_t in(uint16_t port) {
    uint8_t result;
    __asm__ volatile(
        "inb %1, %0" 
        : "=a"(result) 
        : "Nd"(port)
    );
    return result;
}