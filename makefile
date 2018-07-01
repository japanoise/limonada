CFLAGS=$(shell pkg-config --cflags gtk+-2.0 sdl2)
LDFLAGS=$(shell pkg-config --libs gtk+-2.0 sdl2 sdl2_image) -lm
PROGNAME=limonada

all: $(PROGNAME)

debug: CFLAGS+=-g -O0 -v -Q
debug: $(PROGNAME)

$(PROGNAME): slice.o main.o state.o buffer.o gui.o platform.o stb_image.o color.o
	$(CC) -o $@ $^ $(LDFLAGS)

cp437.xpm: cp437.png
	convert $< $@

hsv.h: hsv.png embed
	./embed hsv.png hsv.h

embed: embed.o
	$(CC) -o $@ $^

$(PROGNAME).zip: $(PROGNAME).exe
	zip $@ $^ *.dll

clean:
	rm -rf *.o
	rm -rf $(PROGNAME)
	rm -rf $(PROGNAME).exe
