#include "server.h"
#include "websocket.h"
#include "sha1.h"
#include "base64.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void send_error(FILE *out, const char *protocol, const char *code, const char *reason, const char *body) {
	fprintf(out, "%s %s %s\r\n"
		"Connection: Closed\r\n"
		"\r\n"
	"%s", protocol, code, reason, body);
	fflush(out);
}

void send_400(FILE *out, const char *protocol) {
	send_error(out, protocol, "400", "Bad Request", "");
}

void send_400_pre(FILE *out) {
	send_400(out, "HTTP/1.1");
}

void send_405(FILE *out, const char *protocol) {
	send_error(out, protocol, "405", "Method Not Allowed", "");
}

void send_404(FILE *out, const char *protocol) {
	send_error(out, protocol, "404", "Not Found", "");
}

void send_switching(FILE *out, const char *protocol, const char *websocket_hash) {
	fprintf(out, "%s 101 Switching Protocols\r\n"
		"Upgrade: WebSocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: %s\r\n"
	"\r\n", protocol, websocket_hash);
	fflush(out);
}

void hash_websocket_key(char out[30], const char *key) {
	char *input = asprintfx("%s%s", key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

	char hash[20];
	SHA1(hash, input, strlen(input));
	base64_encode(sizeof(hash), (unsigned char*)hash, 30, out);

	freex(input);
}

int prepare_client(struct websocket_client *client) {
	char line[8192];
	if (!fgets(line, sizeof(line), client->in)) {
		send_400_pre(client->out);
		return -1;
	}

	char method[16];
	char protocol[16];
	if (sscanf(line, "%15s %*s %15s", method, protocol) != 2) {
		send_400_pre(client->out);
		return -1;
	}

	char protocolTest[] = "HTTP/";
	if (strncasecmp(protocolTest, protocol, strlen(protocolTest))) {
		send_400_pre(client->out);
		return -1;
	}

	if (strcasecmp(method, "GET")) {
		send_405(client->out, protocol);
		return -1;
	}

	//printf("METHOD: %s, PROTOCOL: %s\n", method, protocol);

	int upgrade_websocket = 0;
	int connection_upgrade = 0;
	char hash[30];
	hash[0] = '\0';

	while (1) {
		if (!fgets(line, sizeof(line), client->in)) {
			send_400(client->out, protocol);
			return -1;
		}

		if (!strcspn(line, "\r\n")) {
			break;
		}

		char key[8192];
		char value[8192];
		if (sscanf(line, "%[^:]:%s", key, value) != 2) {
			send_400(client->out, protocol);
			return -1;
		}

		if (!strcasecmp(key, "upgrade") && !strcasecmp(value, "websocket")) {
			upgrade_websocket = 1;
		} else if (!strcasecmp(key, "connection") && !strcasecmp(value, "upgrade")) {
			connection_upgrade = 1;
		} else if (!strcasecmp(key, "sec-websocket-key")) {
			hash_websocket_key(hash, value);
		} else if (!strcasecmp(key, "x-forwarded-for")) {
			freex(client->cipaddr);
			client->cipaddr = strdupx(value);
		}

		//printf("HEADER: %s __ %s\n", key, value);
	}

	if (!upgrade_websocket || !connection_upgrade || !hash[0]) {
		send_404(client->out, protocol);
		return -1;
	}

	send_switching(client->out, protocol, hash);
	return 0;
}

void* start_client(void *ptr) {
	struct websocket_client *client = ptr;

	client->in = fdopenx(client->cfd, "r");
	client->out = fdopenx(dupx(client->cfd), "w");

	if (!prepare_client(client)) {
		printf("Open %s\n", client->cipaddr);
		client->ctx->connected(client);

		read_websocket_stream(client);

		printf("Close %s\n", client->cipaddr);
		client->ctx->disconnected(client); // responsible for closing both streams
	} else {
		websocket_close(client);
	}

	return NULL;
}

void websocket_listen(uint16_t port, struct websocket_context *ctx) {
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		fatal("Cannot create socket: %s", strerror(errno));
	}

	int enable = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
		fatal("Cannot set reuseaddr: %s", strerror(errno));
	}

	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sfd, (struct sockaddr*) &saddr, sizeof(saddr)) < 0) {
		fatal("Cannot bind server: %s", strerror(errno));
	}

	if (listen(sfd, 32) < 0) {
		fatal("Cannot listen: %s", strerror(errno));
	}

	printf("Listening on %i\n", port);

	while (1) {
		struct sockaddr_in caddr;
		socklen_t csize = sizeof(caddr);
		int cfd = accept(sfd, (struct sockaddr*) &caddr, &csize);
		if (cfd < 0) {
			fatal("Cannot accept client: %s", strerror(errno));
		}

		struct websocket_client *client = mallocx(sizeof(struct websocket_client));
		client->ctx = ctx;
		client->cipaddr = strdupx(inet_ntoa(caddr.sin_addr));
		client->cfd = cfd;

		pthread_createx(start_client, client);
	}
}

void websocket_close(struct websocket_client *client) {
	fclosex(client->in);
	fclosex(client->out);
	freex(client->cipaddr);
	freex(client);
}
