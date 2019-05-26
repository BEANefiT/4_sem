CC := gcc
CFLAGS := -pthread -std=c99

.INTERMEDIATE: server.o client.o netlib.o

all: server client

%.o: %.c netlib.h

server: server.o netlib.o

client: client.o netlib.o

.PHONY: clean

clean:
	@rm -rf server client
