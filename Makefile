CC = clang
CFLAGS = -Wall -Wextra -O2 -Iinclude
SRC = $(shell find . -name '*.c')
TARGET = bin/out

.PHONY: build clean

build:
	@mkdir -p $(dir $(TARGET))
	@$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: build
	@./$(TARGET) sample.txt
