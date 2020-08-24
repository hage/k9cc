CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

.PHONY: test clean

k9cc: $(OBJS)

$(OBJS): k9cc.h

test: k9cc
	./test.sh

clean:
	rm -f k9cc *.s tmp* *.o a.out
