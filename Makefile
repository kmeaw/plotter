CFLAGS := $(shell pkg-config --cflags sdl SDL_gfx) -W -Wall -g
LIBS := $(shell pkg-config --libs sdl SDL_gfx)
LDFLAGS := -g

main: main.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

.PHONY: clean

clean:
	rm -f main main.o

run: main
	od -An -w2 /dev/urandom | ./main
