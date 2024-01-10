.PHONY: default check-cross-compile
default: all

all:
ifndef CROSS_COMPILE
	CROSS_COMPILE=""
endif

	$(CROSS_COMPILE)gcc aesdsocket.c -o aesdsocket -g -Wall -Werror


clean:
	rm -f *.o writer
