# Use ?= to preserves any previous value of CC
CC ?= $(CROSS_COMPILE)gcc

all:
	$(CC) -o tetris main.c display/ssd1306.c

clean :
	rm -f tetris
