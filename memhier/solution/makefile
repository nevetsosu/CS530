all: main

main: config.c main.c
	gcc -o $@ $^

test: main
	./main
