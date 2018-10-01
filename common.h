#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void fatal(const char *msg, ...);
void* mallocx(size_t size);
char* strdupx(const char *s);
char* asprintfx(const char *fmt, ...);
void freex(void *ptr);
FILE *fdopenx(int fd, const char *mode);
int fclosex(FILE *stream);
int dupx(int fd);
void pthread_createx(void *(*start_routine)(void *), void *arg);

#endif
