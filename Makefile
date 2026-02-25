run: build
	./tui_demo

build:
	cc -std=c11 -o tui_demo main.c tui.c $(shell pkg-config --cflags --libs sdl3 sdl3-ttf)
