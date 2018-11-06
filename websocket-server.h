void fatal(const char *msg, ...);
void *c_malloc(size_t size);
char *c_strdup(const char *s);
void c_free(void *ptr);
void c_pthread_create(void *(*start_routine)(void *), void *arg);

struct websocket_client;

struct websocket_context {
	void (*connected)(struct websocket_client*);
	void (*disconnected)(struct websocket_client*);
	void (*received)(struct websocket_client*, const char*);
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

void websocket_listen(uint16_t port, struct websocket_context *ctx);
void websocket_send(FILE *out, const char *text);
void websocket_close(struct websocket_client *client);
