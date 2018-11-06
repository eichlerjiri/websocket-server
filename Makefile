CFLAGS=-O2 -pedantic -Wall -Wwrite-strings -Wconversion -Wno-unused-function -DENABLE_TRACE=0

websocket-server: phony
	gcc $(CFLAGS) -c -o $@.a $@.c
phony:
