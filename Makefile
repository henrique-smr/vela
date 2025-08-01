TARGET=bin/main
CC=gcc
LD=gcc
DEBUG=-g
OPT=-O0
WARN=-Wall
PTHREAD=-pthread
CCFLAGS=$(DEBUG) $(OPT) $(WARN) $(PTHREAD) -pipe
INCLUDES=-I./src/ -I./vendor/raylib/include/ -I./vendor/fftw/api -I./vendor/miniaudio/ -I./vendor/microui/ -I./vendor/murl/ -I./vendor/raygui/src/
LIBS=-lGL -lm -lpthread -ldl -lrt -lX11
STATIC_LIBS=./vendor/raylib/lib/libraylib.a ./vendor/fftw/.libs/libfftw3.a
LDFLAGS=-export-dynamic $(PTHREAD) $(INCLUDES) $(LIBS)

SRCS = $(wildcard src/*.c)
VENDOR_SRCS = vendor/murl/*.c vendor/microui/*.c
OBJS = $(SRCS:src/%.c=build/%.o) build/murl.o build/microui.o

all: $(OBJS)
	$(LD) -o $(TARGET) $(OBJS) $(STATIC_LIBS) $(LDFLAGS)

build/%.o: src/%.c src/*.h $(VENDOR_SRCS)
	$(CC) -c $(CCFLAGS) $(INCLUDES) $< -o $@

build/murl.o: vendor/murl/murl.c
	$(CC) -c $(CCFLAGS) $(INCLUDES) $< -o $@

build/microui.o: vendor/microui/microui.c
	$(CC) -c $(CCFLAGS) $(INCLUDES) $< -o $@


clean:
	rm -f build/*.o
