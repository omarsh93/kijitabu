.PHONY: build install

build:
	cd build && cmake .. && make

install:
	cp -u build/kijitabu ~/bin
	cp -u images/icon.png ~/.local/share/icons/hicolor/256x256/apps/kijitabu.png
	cp -u conf/kijitabu.desktop ~/.local/share/applications/
