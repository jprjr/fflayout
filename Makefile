.PHONY: all clean

PKGCONFIG = pkg-config
CFLAGS = -Wall -Wextra -g -O2 -fPIC $(shell $(PKGCONFIG) --cflags libavformat libavutil)
LDFLAGS = $(shell $(PKGCONFIG) --libs libavformat libavutil)

SOURCES = \
	src/main.c

OBJS = $(SOURCES:%.c=%.o)

all: fflayout

fflayout: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f fflayout fflayout.exe $(OBJS)
