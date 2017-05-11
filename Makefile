CFLAGS=$(shell sdl-config --cflags)
LDFLAGS=$(shell sdl-config --libs)

main: main.o

.PHONY: clean

clean:
	rm -f main main.o

run: main
	./main
