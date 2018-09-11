#include "websocket.h"
#include "server.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

struct websocket_header {
	unsigned int opcode : 4;
	unsigned int reserved : 3;
	unsigned int fin : 1;
	unsigned int length : 7;
	unsigned int mask : 1;
};

uint16_t reverse16(uint16_t x) {
	return ((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8);
}

uint64_t reverse64(uint64_t x) {
	x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
	x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
	x = (x & 0x00FF00FF00FF00FF) << 8 | (x & 0xFF00FF00FF00FF00) >> 8;
	return x;
}

void send_packet(FILE *out, int opcode, char *text, uint64_t length) {
	int extra_length = 0;

	struct websocket_header h;
	h.opcode = opcode;
	h.reserved = 0;
	h.fin = 1;
	if (length <= 125) {
		h.length = length;
	} else if (length <= 65535) {
		h.length = 126;
		extra_length = 1;
	} else {
		h.length = 127;
		extra_length = 2;
	}
	h.mask = 0;

	fwrite(&h, 2, 1, out);

	if (extra_length == 1) {
		uint16_t val = reverse16((uint16_t) length);
		fwrite(&val, 2, 1, out);
	} else if (extra_length == 2) {
		uint64_t val = reverse64(length);
		fwrite(&val, 8, 1, out);
	}

	fwrite(text, length, 1, out);
	fflush(out);
}

void websocket_send(FILE *out, char *text) {
	send_packet(out, 0x1, text, strlen(text));
}

void send_close(FILE *out, char *text) {
	send_packet(out, 0x8, text, strlen(text));
}

void read_websocket_stream(struct websocket_client *client) {
	char data[8192];
	uint64_t length = 0;
	unsigned int opcode = 0;

	while (1) {
		struct websocket_header h;
		if (fread(&h, 2, 1, client->in) != 1) {
			return;
		}
		//printf("FIN: %i, RESERVED: %i, OPCODE: %i, MASK: %i, LENGTH: %i\n",
		//	h.fin, h.reserved, h.opcode, h.mask, h.length);

		if (h.mask != 1) {
			send_close(client->out, "1002 Protocol error");
			return;
		}

		if (opcode && h.opcode) {
			send_close(client->out, "1002 Protocol error");
			return;
		} else if (!opcode && !h.opcode) {
			send_close(client->out, "1002 Protocol error");
			return;
		}

		if (h.opcode) {
			opcode = h.opcode;
		}

		uint64_t curlength = h.length;
		if (curlength == 126) {
			uint16_t val;
			if (fread(&val, 2, 1, client->in) != 1) {
				return;
			}
			curlength = reverse16(val);
		} else if (curlength == 127) {
			if (fread(&curlength, 8, 1, client->in) != 1) {
				return;
			}
			curlength = reverse64(curlength);
		}

		if (curlength >= sizeof(data) || curlength + length >= sizeof(data)) { // beware overflow
			send_close(client->out, "1009 Message Too Big");
			return;
		}

		char xor_mask[4];
		if (fread(xor_mask, 4, 1, client->in) != 1) {
			return;
		}

		if (fread(data + length, curlength, 1, client->in) != 1) {
			return;
		}

		for (int i = 0; i < curlength; i++) {
			data[length + i] = data[length + i] ^ xor_mask[i % 4];
		}

		length += curlength;

		if (h.fin == 1) {
			if (opcode == 1) {
				data[length] = '\0';
				client->ctx->received(client, data);
			} else if (opcode == 2) {
				// nothing to do
			} else if (opcode == 8) {
				send_close(client->out, "1000 Normal Closure");
			} else if (opcode == 9) {
				send_packet(client->out, 0xA, data, length);
			} else if (opcode == 10) {
				// nothing to do
			} else {
				send_close(client->out, "1002 Protocol error");
				return;
			}

			length = 0;
			opcode = 0;
		}
	}
}
