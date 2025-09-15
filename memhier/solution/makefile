CFLAGS=-Wall -Wextra -ggdb

all: main

main: config.c main.c cache.c util.c
	gcc $(CFLAGS) -o $@ $^

test: main
	./main

clean:
	rm ./main
