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
	x = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
	return x;
}

void send_to_client(FILE *cwrite, int opcode, char *data, uint64_t length) {
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

	fwrite(&h, 2, 1, cwrite);

	if (extra_length == 1) {
		uint16_t val = reverse16((uint16_t) length);
		fwrite(&val, 2, 1, cwrite);
	} else if (extra_length == 2) {
		uint64_t val = reverse64(length);
		fwrite(&val, 8, 1, cwrite);
	}

	fwrite(data, length, 1, cwrite);
	fflush(cwrite);
}

void send_close(FILE *cwrite, char *text) {
	send_to_client(cwrite, 8, text, strlen(text));
}

void read_websocket_stream(FILE *cread, FILE *cwrite, struct websocket_callbacks callbacks, void *userobj1, void *userobj2) {
	char data[8192];
	uint64_t previous_length = 0;
	unsigned int previous_type = 0;

	while (1) {
		struct websocket_header h;
		if (fread(&h, 2, 1, cread) != 1) {
			return;
		}
		printf("FIN: %i, RESERVED: %i, OPCODE: %i, MASK: %i, LENGTH: %i\n", h.fin, h.reserved, h.opcode, h.mask, h.length);

		if (h.mask != 1) {
			send_close(cwrite, "1002 Protocol error");
			return;
		}

		if (previous_type > 0 && h.opcode != 0) {
			send_close(cwrite, "1002 Protocol error");
			return;
		} else if (previous_type == 0 && h.opcode == 0) {
			send_close(cwrite, "1002 Protocol error");
			return;
		}

		if (h.opcode != 0) {
			previous_type = h.opcode;
		}

		uint64_t length = h.length;
		if (length == 126) {
			uint16_t val;
			if (fread(&val, 2, 1, cread) != 1) {
				return;
			}
			length = reverse16(val);
		} else if (length == 127) {
			if (fread(&length, 8, 1, cread) != 1) {
				return;
			}
			length = reverse64(length);
		}

		if (length >= sizeof(data) || length + previous_length >= sizeof(data)) { // beware overflow
			send_close(cwrite, "1009 Message Too Big");
			return;
		}

		char xor_mask[4];
		if (fread(xor_mask, 4, 1, cread) != 1) {
			return;
		}

		if (fread(data + previous_length, length, 1, cread) != 1) {
			return;
		}

		for (int i = 0; i < length; i++) {
			data[previous_length + i] = data[previous_length + i] ^ xor_mask[i % 4];
		}

		previous_length += length;

		if (h.fin == 1) {
			if (previous_type == 1) {
				data[previous_length] = '\0';

				callbacks.message_received(userobj1, userobj2, data);
			} else if (previous_type == 2) {
				// nothing to do
			} else if (previous_type == 8) {
				send_close(cwrite, "1000 Normal Closure");
			} else if (previous_type == 9) {
				send_to_client(cwrite, 0xA, data, previous_length);
			} else if (previous_type == 10) {
				// nothing to do
			} else {
				send_close(cwrite, "1002 Protocol error");
				return;
			}

			previous_length = 0;
			previous_type = 0;
		}
	}
}
