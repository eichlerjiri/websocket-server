#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void fatal(char *msg, ...);
void* calloc_x(size_t nmemb, size_t size);
void pthread_mutex_init_x(pthread_mutex_t *restrict mutex);
void pthread_mutex_lock_x(pthread_mutex_t *mutex);
void pthread_mutex_unlock_x(pthread_mutex_t *mutex);
void pthread_cond_init_x(pthread_cond_t *restrict cond);
void pthread_cond_wait_x(pthread_cond_t *cond, pthread_mutex_t *lock);
void pthread_cond_signal_x(pthread_cond_t *cond);
FILE *fdopen_x(int fd, const char *mode);
void pthread_create_x(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
int dup_x(int fildes);

#endif
