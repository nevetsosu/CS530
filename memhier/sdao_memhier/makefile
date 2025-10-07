CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -ggdb

INC_DIR = ./include
SRC_DIR = ./src
TARGET = memhier

SRCS = $(wildcard $(SRC_DIR)/*.c)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $^

test: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

clean:
	rm -f $(TARGET)

