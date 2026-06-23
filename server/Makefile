CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -lrt -pthread
TARGET = aesdsocket

all: $(TARGET)

default: $(TARGET)

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
