CFLAGS=$(shell pkg-config --cflags sdl SDL_gfx) -W -Wall -g
LDFLAGS=$(shell pkg-config --libs sdl SDL_gfx) -g

main: main.o

.PHONY: clean

clean:
	rm -f main main.o

run: main
	od /dev/urandom | sed -e 's/[0-9]* \?/0.&\n/g' | ./main
