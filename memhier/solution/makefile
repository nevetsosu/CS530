CFLAGS=-Wall -Wextra -ggdb

all: main

main: config.c main.c cache.c util.c
	gcc $(CFLAGS) -o $@ $^

test: main
	./main

valgrind: main
	valgrind --leak-check=full --show-leak-kinds=all ./main

clean:
	rm ./main
