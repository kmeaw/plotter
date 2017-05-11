CFLAGS=$(shell sdl-config --cflags) -W -Wall -g
LDFLAGS=$(shell sdl-config --libs) -g

main: main.o

.PHONY: clean

clean:
	rm -f main main.o

run: main
	od /dev/urandom | sed -e 's/[0-9]* \?/0.&\n/g' | ./main
