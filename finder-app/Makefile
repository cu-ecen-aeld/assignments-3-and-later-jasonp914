.PHONY: default check-cross-compile
default:
ifndef CROSS_COMPILE
	CROSS_COMPILE=""
endif

	$(CROSS_COMPILE)gcc writer.c -o writer

clean:
	rm -f *.o writer
