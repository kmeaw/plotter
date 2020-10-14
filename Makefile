CFLAGS := $(shell pkg-config --cflags sdl SDL_gfx) -W -Wall -g
LDLIBS := $(shell pkg-config --libs sdl SDL_gfx)
LDFLAGS := -g

main: main.o

.PHONY: clean

clean:
	rm -f main main.o

run: main
	od -An -w2 /dev/urandom | ./main
