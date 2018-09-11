#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>

struct websocket_client;

struct websocket_context {
	void (*connected)(struct websocket_client*);
	void (*disconnected)(struct websocket_client*);
	void (*received)(struct websocket_client*, char*);
	void *ex;
};

struct websocket_client {
	struct websocket_context *ctx;
	char *cipaddr;
	int cfd;
	FILE *in;
	FILE *out;
	void *ex;
};

void websocket_listen(int port, struct websocket_context *ctx);
void websocket_close(struct websocket_client *client);

#endif
