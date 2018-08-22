OUTNAME=websocket-server.a

all: server.o websocket.o sha1.o base64.o common.o
	ar rcs $(OUTNAME) $^

%.o : %.c
	gcc -O2 -Wall -pedantic -c -o $@ $<

clean:
	rm *.o $(OUTNAME)
