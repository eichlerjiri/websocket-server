#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "server.h"
#include <stdio.h>
#include <stdint.h>

void websocket_send(FILE *out, char *text);

void read_websocket_stream(struct websocket_client *client);

#endif
