CFLAGS=-O2 -ffunction-sections -fdata-sections -Wl,--gc-sections -pedantic -Wall -Wwrite-strings -Wconversion
OUTNAME=websocket-server.a

all: common.o trace.o sha1.o base64.o server.o websocket.o
	ar rcs $(OUTNAME) $^

%.o : %.c
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm *.o $(OUTNAME)
