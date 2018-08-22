#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "server.h"
#include <stdio.h>
#include <stdint.h>

void read_websocket_stream(FILE *cread, FILE  *cwrite, struct websocket_callbacks callbacks, void *userobj1, void *userobj2);
void send_to_client(FILE *cwrite, int opcode, char *data, uint64_t length);

#endif
