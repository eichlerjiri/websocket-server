#include "server.h"
#include "websocket.h"
#include "sha1.h"
#include "base64.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>

struct to_recv_thread {
	struct websocket_callbacks callbacks;
	int clientfd;
	void *userobj1;
};

void send_error_response(FILE *cwrite, char *protocol, char *code, char *reason, char *body) {
	fprintf(cwrite, "%s %s %s\r\n"
		"Connection: Closed\r\n"
		"\r\n"
	"%s", protocol, code, reason, body);
	fflush(cwrite);
}

void send_bad_request(FILE *cwrite, char *protocol) {
	send_error_response(cwrite, protocol, "400", "Bad Request", "");
}

void send_bad_request_pre(FILE *cwrite) {
	send_bad_request(cwrite, "HTTP/1.1");
}

void send_method_not_allowed(FILE *cwrite, char *protocol) {
	send_error_response(cwrite, protocol, "405", "Method Not Allowed", "");
}

void send_not_found(FILE *cwrite, char *protocol) {
	send_error_response(cwrite, protocol, "404", "Not Found", "");
}

void send_switching(FILE *cwrite, char *protocol, char *websocket_hash) {
	fprintf(cwrite, "%s 101 Switching Protocols\r\n"
		"Upgrade: WebSocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: %s\r\n"
	"\r\n", protocol, websocket_hash);
	fflush(cwrite);
}

int read_line(FILE *cread, char *buffer, int length) {
	if (!fgets(buffer, length, cread)) {
		return -1;
	}

	char *pos1 = strchr(buffer, '\r');
	char *pos2 = strchr(buffer, '\n');
	if (!pos1 && !pos2) {
		return -1;
	}

	if (pos1) {
		*pos1 = '\0';
	}
	if (pos2) {
		*pos2 = '\0';
	}

	return 0;
}

void prepare_websocket_hash(char *buffer, int buffer_length, char *key) {
	char temp[8192 + 100];
	sprintf(temp, "%s%s", key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

	char tempsha1[21];
	SHA1(tempsha1, temp, strlen(temp));

	base64_encode(20, (unsigned char*)tempsha1, buffer_length, buffer);
}

void receive_client(FILE *cread, FILE *cwrite, struct websocket_callbacks callbacks, void *userobj1) {
	char line[8192];
	if (read_line(cread, line, sizeof(line)) < 0) {
		send_bad_request_pre(cwrite);
		return;
	}

	char method[16];
	char protocol[16];
	if (sscanf(line, "%15s %*s %15s", method, protocol) < 2) {
		send_bad_request_pre(cwrite);
		return;
	}

	char protocolTest[] = "HTTP/";
	if (strncmp(protocolTest, protocol, strlen(protocolTest)) != 0) {
		send_bad_request_pre(cwrite);
		return;
	}

	if (strcmp(method, "GET") != 0) {
		send_method_not_allowed(cwrite, protocol);
		return;
	}

	printf("METHOD: %s, PROTOCOL: %s\n", method, protocol);

	int upgrade_websocket = 0;
	int connection_upgrade = 0;
	char websocket_hash[100];
	websocket_hash[0] = '\0';

	while (1) {
		if (read_line(cread, line, sizeof(line)) < 0) {
			send_bad_request(cwrite, protocol);
			return;
		}

		if (strlen(line) == 0) {
			break;
		}

		char key[8192];
		char value[8192];
		if (sscanf(line, "%[^:]:%s", key, value) < 2) {
			send_bad_request(cwrite, protocol);
			return;
		}

		if (strcasecmp(key, "upgrade") == 0 && strcasecmp(value, "websocket") == 0) {
			upgrade_websocket = 1;
		} else if (strcasecmp(key, "connection") == 0 && strcasecmp(value, "upgrade") == 0) {
			connection_upgrade = 1;
		} else if (strcasecmp(key, "sec-websocket-key") == 0) {
			prepare_websocket_hash(websocket_hash, sizeof(websocket_hash), value);
		}

		printf("HEADER: %s __ %s\n", key, value);
	}

	if (upgrade_websocket == 0 || connection_upgrade == 0 || websocket_hash[0] == '\0') {
		send_not_found(cwrite, protocol);
		return;
	}

	send_switching(cwrite, protocol, websocket_hash);

	void *userobj2 = callbacks.client_connected(userobj1, cwrite);
	read_websocket_stream(cread, cwrite, callbacks, userobj1, userobj2);
	callbacks.client_disconnected(userobj1, userobj2);
}

void* receive_client_thread(void *ptr) {
	struct to_recv_thread *d = ptr;
	int clientfd = d->clientfd;
	struct websocket_callbacks callbacks = d->callbacks;
	void *userobj1 = d->userobj1;
	free(ptr);

	FILE *cread = fdopen_x(clientfd, "r");
	FILE *cwrite = fdopen_x(dup_x(clientfd), "w");
	receive_client(cread, cwrite, callbacks, userobj1);
	fclose(cread); // not closing write, app will do that

	return NULL;
}

void receive_client_threaded(int clientfd, struct websocket_callbacks callbacks, void *userobj1) {
	struct to_recv_thread *ptr = calloc_x(1, sizeof(struct to_recv_thread));
	ptr->clientfd = clientfd;
	ptr->callbacks = callbacks;
	ptr->userobj1 = userobj1;

	pthread_t thread;
	pthread_create_x(&thread, NULL, receive_client_thread, ptr);
}

void start_server(int port, struct websocket_callbacks callbacks, void *userobj1) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fatal("Cannot create socket: %s", strerror(errno));
	}

	int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
		fatal("Cannot set reuseaddr: %s", strerror(errno));
	}

	struct sockaddr_in serv_addr = {0};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		fatal("Cannot bind server: %s", strerror(errno));
	}

	if (listen(sockfd, 32) < 0) {
		fatal("Cannot listen: %s", strerror(errno));
	}
	printf("Listening on %i\n", port);

	while (1) {
		struct sockaddr_in cli_addr = {0};
		unsigned int cli_len = sizeof(cli_addr);
		int clientfd = accept(sockfd, (struct sockaddr*) &cli_addr, &cli_len);
		if (clientfd < 0) {
			fatal("Cannot accept client: %s", strerror(errno));
		}
		receive_client_threaded(clientfd, callbacks, userobj1);
	}
}
