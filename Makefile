.PHONY: build install

build:
	cd build && cmake .. && make

install:
	cp build/kijitabu ~/bin
