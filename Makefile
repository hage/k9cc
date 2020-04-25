.PHONY: sh build

sh:
	docker run --rm -it -v $(shell pwd):/home/user compilerbook

build:
	docker build -t compilerbook https://www.sigbus.info/compilerbook/Dockerfile
