OUTNAME=websocket-server.a

all: common.o trace.o sha1.o base64.o server.o websocket.o
	ar rcs $(OUTNAME) $^

%.o : %.c
	gcc -O2 -Wall -pedantic -c -o $@ $<

clean:
	rm *.o $(OUTNAME)
