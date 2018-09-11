#include "trace.h"
#include <stdio.h>

FILE *ftrace;

void trace_print(char op, char *type, void *id, char *comment) {
	if (!ftrace) {
		ftrace = fopen("trace.txt", "w");
		if (!ftrace) {
			return;
		}
	}

	fprintf(ftrace, "%c %s %p %s\n", op, type, id, comment);
	fflush(ftrace);
}

void trace_start(char *type, void *id, char *comment) {
	trace_print('A', type, id, comment);
}

void trace_end(char *type, void* id, char *comment) {
	trace_print('F', type, id, comment);
}
