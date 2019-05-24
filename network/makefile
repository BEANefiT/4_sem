CC := gcc
CFLAGS := -pthread -std=c99

all: server client

%.o: %.c netlib.h

server: server.o

client: client.o

.PHONY: clean

clean:
	rm -rf server client *.o
