#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"

struct FramebufferState framebuffer_state = {
    .row = 0, 
    .col = 0, 
    .fg = 0xF, 
    .bg = 0
};

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    uint16_t pos = r * BUFFER_WIDTH + c; 
    out(CURSOR_PORT_CMD, 0x0F);
	out(CURSOR_PORT_DATA, (uint8_t) (pos & 0xFF));
	out(CURSOR_PORT_CMD, 0x0E);
	out(CURSOR_PORT_DATA, (uint8_t) ((pos >> 8) & 0xFF));
    framebuffer_state.row = r; 
    framebuffer_state.col = c;
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    uint16_t idx = (row * BUFFER_WIDTH) +  col; 
    uint16_t *where = (uint16_t *)FRAMEBUFFER_MEMORY_OFFSET + idx;
    uint16_t attrib = (bg << 4) | (fg & 0x0F);
    *where = c | (attrib << 8);
}

void framebuffer_clear(void) {
    size_t framebuffer_size = BUFFER_WIDTH * BUFFER_HEIGHT + sizeof(FRAMEBUFFER_MEMORY_OFFSET);
    memset(FRAMEBUFFER_MEMORY_OFFSET, 0, framebuffer_size);
}