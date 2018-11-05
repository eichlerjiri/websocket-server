ENABLE_TRACE = 0

ifneq ($(ENABLE_TRACE), 0)
TRACE_FILE=trace.o
TRACE_FLAG=-DENABLE_TRACE=1
else
TRACE_FILE=
TRACE_FLAG=
endif

CFLAGS=-O2 -ffunction-sections -fdata-sections -Wl,--gc-sections -pedantic -Wall -Wwrite-strings -Wconversion
OUTNAME=websocket-server.a

all: common.o $(TRACE_FILE) sha1.o base64.o server.o websocket.o
	ar rcs $(OUTNAME) $^

%.o : %.c
	gcc $(CFLAGS) $(TRACE_FLAG) -c -o $@ $<

clean:
	rm *.o $(OUTNAME)
