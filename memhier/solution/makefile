CFLAGS=-Wall -Wextra -ggdb
OBJS = config.c main.c cache.c util.c palloc.c ptable.c

all: main

main: $(OBJS)
	gcc $(CFLAGS) -o $@ $^

test: main
	./main

valgrind: main
	valgrind --leak-check=full --show-leak-kinds=all ./main

clean:
	rm ./main
