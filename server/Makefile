.PHONY: default check-cross-compile

CC = gcc
OBJ = aesdsocket
HEADER = aesdsocket.h
CFLAGS = -g -Wall -Werror -pthread


default: all

all: aesdsocket.c $(HEADER)
ifndef CROSS_COMPILE
	CROSS_COMPILE=""
endif

	#$(CROSS_COMPILE)gcc aesdsocket.c -o aesdsocket -g -Wall -Werror
	$(CROSS_COMPILE)$(CC) aesdsocket.c -o $(OBJ) $(CFLAGS) 
	#$(CROSS_COMPILE)$(CC) $(CFLAGS) $< -o $(OBJ)  


clean:
	rm -f *.o writer
