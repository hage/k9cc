CFLAGS=-std=c11 -g -static

k9cc: k9cc.c

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
