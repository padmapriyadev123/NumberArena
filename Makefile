# Optional Makefile (Linux, or Windows with MSYS2 'make').  Usage: make | make run | make clean
CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -g -Iinclude
SRC     = main.c $(wildcard src/*.c)
TARGET  = NUMBER_ARENA

$(TARGET): $(SRC) $(wildcard include/*.h)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe
