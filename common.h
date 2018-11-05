#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void fatal(const char *msg, ...);
void* c_malloc(size_t size);
char* c_strdup(const char *s);
char* c_asprintf(const char *fmt, ...);
void c_free(void *ptr);
FILE *c_fdopen(int fd, const char *mode);
int c_fclose(FILE *stream);
int c_dup(int fd);
void c_pthread_create(void *(*start_routine)(void *), void *arg);

#endif
