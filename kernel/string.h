// for assignment 2 q1, xv6 header for string functions

#ifndef STRING_H
#define STRING_H

#include "types.h"

int memcmp(const void *, const void *, uint);
void *memmove(void *, const void *, uint);
void *memset(void *, int, uint);
char *strchr(const char *, char);
int strcmp(const char *, const char *);
char *strcpy(char *, const char *);
int strlen(const char *);
void *memcpy(void *, const void *, uint);

#endif
