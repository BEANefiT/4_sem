CC := gcc
CFLAGS := -pthread -std=c99

.INTERMEDIATE: server.o client.o netlib.o ../scaler/scaler.o

all: server client

%.o: %.c netlib.h ../scaler/scaler.h

server: server.o netlib.o ../scaler/scaler.o integrator.h
	$(CC) $(CFLAGS) $^ -o $@

client: client.o netlib.o ../scaler/scaler.o integrator.h
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	@rm -rf server client
