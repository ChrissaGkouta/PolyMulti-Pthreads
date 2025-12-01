CC = gcc
CFLAGS = -Wall -O3 
LIBS = -pthread 
TARGET = poly_mult

all: $(TARGET)

$(TARGET): poly_mult.c
	$(CC) $(CFLAGS) -o $(TARGET) poly_mult.c $(LIBS)

clean:
	rm -f $(TARGET)