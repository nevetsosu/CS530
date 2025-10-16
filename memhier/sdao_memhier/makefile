CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -ggdb

INC_DIR = ./include
SRC_DIR = ./src
TARGET = memhier

SRCS = $(wildcard $(SRC_DIR)/*.c)

all: build

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $^

run: $(TARGET)
	./$< < test.dat

build: $(TARGET)

test: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

clean:
	rm -f $(TARGET)

