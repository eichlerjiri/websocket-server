#ifndef TRACE_H
#define TRACE_H

void trace_start(char *type, void *id, char *comment);
void trace_end(char *type, void *id, char *comment);

#endif
