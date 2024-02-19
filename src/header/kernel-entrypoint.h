#ifndef _KERNEL_ENTRYPOINT
#define _KERNEL_ENTRYPOINT

#include "header/cpu/gdt.h"

/**
 * Load GDT from gdtr and complete init for protected mode. This procedure implemented in asm.
 * Note: This procedure will not activate CPU interrupt flag
 * 
 * @param gdtr Pointer to already defined & initialized GDTR
 * @warning Invalid address / definition of GDT will cause bootloop after calling this procedure.
 */
extern void load_gdt(struct GDTR *gdtr);

#endif