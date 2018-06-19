CFLAGS=$(shell sdl2-config --cflags)
LDFLAGS=$(shell sdl2-config --libs) -lSDL2_image
PROGNAME=spr

all: $(PROGNAME)

$(PROGNAME): slice.o main.o cp437.xpm
	$(CC) -o $@ main.o slice.o $(LDFLAGS)

cp437.xpm: cp437.png
	convert $< $@

clean:
	rm -rf *.o
	rm -rf $(PROGNAME)
