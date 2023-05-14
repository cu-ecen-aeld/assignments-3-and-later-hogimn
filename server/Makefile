# Use ?= to preserves any previous value of CC
CC ?= $(CROSS_COMPILE)gcc

all: aesdsocket
default: aesdsocket

aesdsocket:
	$(CC) -o aesdsocket aesdsocket.c

clean :
	rm -f aesdsocket
