#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>

struct websocket_callbacks {
	void *(*client_connected)(void*, FILE*);
	void (*client_disconnected)(void*, void*);
	void (*message_received)(void*, void*, char*);
};

void start_server(int port, struct websocket_callbacks callbacks, void *userobj1);

#endif
