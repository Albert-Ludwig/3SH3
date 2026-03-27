CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: main

main: main.o fs_indexed.o
	$(CC) $(CFLAGS) -o main main.o fs_indexed.o

main.o: main.c fs_indexed.h
	$(CC) $(CFLAGS) -c main.c

fs_indexed.o: fs_indexed.c fs_indexed.h
	$(CC) $(CFLAGS) -c fs_indexed.c

run: main
	./main

clean:
	rm -f *.o main