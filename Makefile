# slopOS Makefile - Simulator Build

CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = slopOS

all: $(TARGET)

$(TARGET): src/slopos.c
	$(CC) $(CFLAGS) src/slopos.c -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
