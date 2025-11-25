CC=gcc
CFLAGS=-O3 -Wall -Wpedantic -Wextra -Iinclude -ggdb 
SRCS := $(wildcard *.c)
# OBJS := $(basename $(SRC))

all: build

# Aliases for 530 grading
build: dynamsched
run: test

dynamsched: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

test: dynamsched
	./dynamsched < ./traces/trace2.dat

valgrind: dynamsched
	valgrind --leak-check=full ./dynamsched ../dynamsched/config.txt < ../dynamsched/trace2.dat

# generate compile_commands.json
compile_commands.json:
	@echo "[" > compile_commands.json
	@first=1; \
	for src in $(SRCS); do \
		if [ $$first -eq 1 ]; then first=0; else echo "," >> compile_commands.json; fi; \
		echo "{ \"file\": \"$$src\", \"directory\": \"$(CURDIR)\", \"command\": \"$(CC) $(CFLAGS) -c $$src\" }" >> compile_commands.json; \
	done
	@echo "]" >> compile_commands.json
clean:
	rm -f ./dynamsched ./compile_commands.json
