CC := gcc
CFLAGS := -fprofile-arcs -ftest-coverage
GCOV := faultinj.gcda faultinj.gcno list.gcda list.gcno main.gcda\
		main.gcno main.c.gcov list.c.gcov

.INTERMEDIATE: list.o faultinj.o

all: main

main: main.c faultinj.o list.o
#$(CC) -o $@ $^ && ./main && gcov list.c
list.o: list.c list.h faultinj.o

faultinj.o: faultinj.h

.PHONY: clean

clean:
	rm -rf $(GCOV) main out log
