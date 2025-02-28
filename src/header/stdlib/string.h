#ifndef _STRING_H_
#define _STRING_H_

#include <stdint.h>
#include <stddef.h>
#include "stdtype.h"

/**
 * C standard memset, check man memset or
 * https://man7.org/linux/man-pages/man3/memset.3.html for more details
 * 
 * @param s Pointer to memory area to set
 * @param c Constant byte value for filling memory area
 * @param n Memory area size in byte 
 * 
 * @return Pointer s
*/
void* memset(void *s, int c, size_t n);

/**
 * C standard memcpy, check man memcpy or
 * https://man7.org/linux/man-pages/man3/memcpy.3.html for more details
 * 
 * @param dest Starting location for memory area to set
 * @param src Pointer to source memory
 * @param n Memory area size in byte 
 * 
 * @return Pointer dest
*/
void* memcpy(void* restrict dest, const void* restrict src, size_t n);

/**
 * C standard memcmp, check man memcmp or
 * https://man7.org/linux/man-pages/man3/memcmp.3.html for more details
 * 
 * @param s1 Pointer to first memory area
 * @param s2 Pointer to second memory area
 * @param n Memory area size in byte 
 * 
 * @return Integer as error code, zero for equality, non-zero for inequality
*/
int memcmp(const void *s1, const void *s2, size_t n);

/**
 * C standard memmove, check man memmove or
 * https://man7.org/linux/man-pages/man3/memmove.3.html for more details
 * 
 * @param dest Pointer to destination memory
 * @param src Pointer to source memory
 * @param n Memory area size in byte 
 * 
 * @return Pointer dest
*/
void *memmove(void *dest, const void *src, size_t n);

int strcmp(const char *stra, const char *strb, int size);

char *strcpy(const char *src, char *dest);

int strlen(const char *str);

boolean is_equal(const char *stra, const char *strb);

void append(const char *stra, const char *strb, char *destination);

void append3(const char *stra, const char *strb, const char *strc, char *destination);

void append_path(const char *stra, const char *strb, char *destination);

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 *
 * This is a modified version with base parameter removed
 * Modified by Akbar
 */
void itoa(int32_t value, char *result);

#endif