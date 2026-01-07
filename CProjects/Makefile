CC=gcc
CFLAGS=-std=c99 -Wall
LIBS=-lncurses

# The name of the executable
TARGET=tetris

all: $(TARGET)

$(TARGET): tetris.c
	$(CC) $(CFLAGS) -o $(TARGET) tetris.c $(LIBS)

clean:
	rm -f $(TARGET)
