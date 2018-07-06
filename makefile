CFLAGS=$(shell pkg-config --cflags gtk+-2.0) $(shell sdl2-config --cflags)
LDFLAGS=$(shell pkg-config --libs gtk+-2.0) $(shell sdl2-config --libs) -lSDL2_image -lm
PROGNAME=limonada
OBJECTS=slice.o main.o state.o buffer.o gui.o platform.o stb_image.o color.o

ifeq ($(OS),Windows_NT)
	OBJECTS+= res.o
endif

all: $(PROGNAME)

debug: CFLAGS+=-g -O0 -v -Q
debug: $(PROGNAME)

$(PROGNAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

cp437.xpm: cp437.png
	convert $< $@

hsv.h: hsv.png embed
	./embed hsv.png hsv.h

res.o: res.rc
	windres res.rc res.o

embed: embed.o
	$(CC) -o $@ $^

$(PROGNAME).zip: $(PROGNAME).exe
	zip $@ $^ *.dll

clean:
	rm -rf *.o
	rm -rf $(PROGNAME)
	rm -rf $(PROGNAME).exe
