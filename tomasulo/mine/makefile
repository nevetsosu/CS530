CC=gcc
CFLAGS=-O3 -Wall -Wpedantic -Wextra -Iinclude
SRCS := $(wildcard *.c)
# OBJS := $(basename $(SRC))

all: build

# For 530 grading
build: tomasulo

tomasulo: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

test: tomasulo
	./tomasulo ../dynamsched/config.txt < ../dynamsched/trace2.dat

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
	rm -f ./tomasulo ./compile_commands.json
