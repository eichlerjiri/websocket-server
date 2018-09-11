#include "common.h"
#include "trace.h"
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

void* callocx(size_t nmemb, size_t size) {
	void *ret = calloc(nmemb, size);
	if (!ret) {
		fatal("Cannot calloc %li x %li: %s", nmemb, size, strerror(errno));
	}
#ifdef TRACE_H
	trace_start("MEM", ret, "calloc");
#endif
	return ret;
}

char* asprintfx(const char *fmt, ...) {
	va_list valist;
	va_start(valist, fmt);

	char *ret;
	if (vasprintf(&ret, fmt, valist) < 0) {
		fatal("Cannot asprintf %s: %s", fmt, strerror(errno));
	}

	va_end(valist);
#ifdef TRACE_H
	trace_start("MEM", ret, "asprintf");
#endif
	return ret;
}

void freex(void *ptr) {
#ifdef TRACE_H
	trace_end("MEM", ptr, "free");
#endif
	free(ptr);
}

FILE *fdopenx(int fd, const char *mode) {
	FILE *ret = fdopen(fd, mode);
	if (!ret) {
		fatal("Cannot fdopen %i: %s", fd, strerror(errno));
	}
#ifdef TRACE_H
	trace_start("FIL", ret, "fdopen");
#endif
	return ret;
}

int fclosex(FILE *stream) {
#ifdef TRACE_H
	trace_end("FIL", stream, "fclose");
#endif
	int ret = fclose(stream);
	if (ret) {
		fatal("Cannot fclose: %s", strerror(errno));
	}
	return ret;
}

int dupx(int fd) {
	int ret = dup(fd);
	if (ret < 0) {
		fatal("Cannot dup %i: %s", fd, strerror(errno));
	}
	return ret;
}

void pthread_createx(void *(*start_routine)(void *), void *arg) {
	pthread_t thread;

	int ret = pthread_create(&thread, NULL, start_routine, arg);
	if (ret) {
		fatal("Cannot pthread_create: %s", strerror(ret));
	}
}
