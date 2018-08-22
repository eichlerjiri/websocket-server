#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

void fatal(char *msg, ...) {
	va_list valist;
	va_start(valist, msg);

	fprintf(stderr, "\033[91m");
	vfprintf(stderr, msg, valist);
	fprintf(stderr, "\033[0m\n");

	va_end(valist);
	exit(1);
}

void* calloc_x(size_t nmemb, size_t size) {
	void *ret = calloc(nmemb, size);
	if (ret == NULL) {
		fatal("Cannot alocate memory: %li x %li", nmemb, size);
	}
	return ret;
}

void pthread_mutex_init_x(pthread_mutex_t *restrict mutex) {
	int ret = pthread_mutex_init(mutex, NULL);
	if (ret != 0) {
		fatal("Cannot create mutex: %i", ret);
	}
}

void pthread_mutex_lock_x(pthread_mutex_t *mutex) {
	int ret = pthread_mutex_lock(mutex);
	if (ret != 0) {
		fatal("Cannot lock mutex: %i", ret);
	}
}

void pthread_mutex_unlock_x(pthread_mutex_t *mutex) {
	int ret = pthread_mutex_unlock(mutex);
	if (ret != 0) {
		fatal("Cannot unlock mutex: %i", ret);
	}
}

void pthread_cond_init_x(pthread_cond_t *restrict cond) {
	int ret = pthread_cond_init(cond, NULL);
	if (ret != 0) {
		fatal("Cannot create cond: %i", ret);
	}
}

void pthread_cond_wait_x(pthread_cond_t *cond, pthread_mutex_t *lock) {
	int ret = pthread_cond_wait(cond, lock);
	if (ret != 0) {
		fatal("Cannot wait on cond: %i", ret);
	}
}

void pthread_cond_signal_x(pthread_cond_t *cond) {
	int ret = pthread_cond_signal(cond);
	if (ret != 0) {
		fatal("Cannot wait on cond: %i", ret);
	}
}

FILE *fdopen_x(int fd, const char *mode) {
	FILE *ret = fdopen(fd, mode);
	if (ret == NULL) {
		fatal("Cannot fdopen socket: %s", strerror(errno));
	}
	return ret;
}

int dup_x(int fd) {
	int ret = dup(fd);
	if (ret < 0) {
		fatal("Cannot dup fd: %s", strerror(errno));
	}
	return ret;
}

void pthread_create_x(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
	int ret = pthread_create(thread, attr, start_routine, arg);
	if (ret != 0) {
		fatal("Cannot create thread: %i", ret);
	}
}
