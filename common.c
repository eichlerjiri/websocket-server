#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>

#if ENABLE_TRACE
#include "trace.c"
#endif

void fatal(const char *msg, ...) {
	va_list valist;
	va_start(valist, msg);

	fprintf(stderr, "\033[91m");
	vfprintf(stderr, msg, valist);
	fprintf(stderr, "\033[0m\n");

	va_end(valist);
	exit(2);
}

void *c_malloc(size_t size) {
	void *ret = malloc(size);
	if (!ret) {
		fatal("Cannot malloc %li: %s", size, strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("MEM", ret, "malloc");
#endif
	return ret;
}

char *c_strdup(const char *s) {
	char *ret = strdup(s);
	if (!ret) {
		fatal("Cannot strdup: %s", strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("MEM", ret, "strdup");
#endif
	return ret;
}

static char *c_asprintf(const char *fmt, ...) {
	va_list valist;
	va_start(valist, fmt);

	char *ret;
	if (vasprintf(&ret, fmt, valist) < 0) {
		fatal("Cannot asprintf %s: %s", fmt, strerror(errno));
	}

	va_end(valist);
#if ENABLE_TRACE
	trace_start("MEM", ret, "asprintf");
#endif
	return ret;
}

void c_free(void *ptr) {
#if ENABLE_TRACE
	if (ptr) {
		trace_end("MEM", ptr, "free");
	}
#endif
	free(ptr);
}

static FILE *c_fdopen(int fd, const char *mode) {
	FILE *ret = fdopen(fd, mode);
	if (!ret) {
		fatal("Cannot fdopen %i: %s", fd, strerror(errno));
	}
#if ENABLE_TRACE
	trace_start("FIL", ret, "fdopen");
#endif
	return ret;
}

static int c_fclose(FILE *stream) {
#if ENABLE_TRACE
	trace_end("FIL", stream, "fclose");
#endif
	int ret = fclose(stream);
	if (ret) {
		fatal("Cannot fclose: %s", strerror(errno));
	}
	return ret;
}

static int c_dup(int fd) {
	int ret = dup(fd);
	if (ret < 0) {
		fatal("Cannot dup %i: %s", fd, strerror(errno));
	}
	return ret;
}

void c_pthread_create(void *(*start_routine)(void *), void *arg) {
	pthread_t thread;

	int ret = pthread_create(&thread, NULL, start_routine, arg);
	if (ret) {
		fatal("Cannot pthread_create: %s", strerror(ret));
	}
}
