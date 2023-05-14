# Use ?= to preserves any previous value of CC
CC ?= $(CROSS_COMPILE)gcc
LDFLAGS ?= -lpthread

all: aesdsocket
default: aesdsocket

aesdsocket:
	$(CC) -o aesdsocket aesdsocket.c $(LDFLAGS)

clean :
	rm -f aesdsocket
