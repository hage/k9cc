CFLAGS=-std=c11 -g -static

.PHONY: test clean

k9cc: k9cc.c

test: k9cc
	./test.sh

clean:
	rm -f k9cc *.s tmp*
