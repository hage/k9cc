CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

k9cc: $(OBJS)
	$(CC) -o k9cc $(OBJS) $(LDFLAGS)

$(OBJS): k9cc.h

test: k9cc
	./test.sh

clean:
	rm -f k9cc *.o *~ tmp*


################ outside of Docker
sh:
	docker run --rm -it -v $(shell pwd):/home/user compilerbook

build:
	docker build -t compilerbook https://www.sigbus.info/compilerbook/Dockerfile

.PHONY: sh build test clean
