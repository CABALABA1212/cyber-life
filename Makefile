CC = gcc
CFLAGS = -Wall -O3
SRC = main.c
TARGET = main

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    LDFLAGS = -lraylib -lm -ldl -lpthread -lGL -lrt -lX11
endif

ifeq ($(OS),Windows_NT)
    LDFLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm -lrpcrt4
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

