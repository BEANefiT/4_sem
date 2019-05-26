CC := gcc
CFLAGS := -pthread -std=c99

.INTERMEDIATE: scaler.o main.o

all: main

%.o: %.c scaler.h

main: scaler.o main.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	@rm -rf main
