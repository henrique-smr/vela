TARGET=bin/main
CC=gcc
LD=gcc
DEBUG=-g
OPT=-O0
WARN=-Wall
PTHREAD=-pthread
CCFLAGS=$(DEBUG) $(OPT) $(WARN) $(PTHREAD) -pipe
INCLUDES=-I./src/ -I./vendor/sds/ -I./vendor/raylib/include/ -I./vendor/fftw/api -I./vendor/miniaudio/ -I./vendor/raygui/src/
LIBS=-lGL -lm -lpthread -ldl -lrt -lX11
STATIC_LIBS=./vendor/raylib/lib/libraylib.a ./vendor/fftw/.libs/libfftw3.a
LDFLAGS=-export-dynamic $(PTHREAD) $(INCLUDES) $(LIBS)

SRCS = $(wildcard src/*.c)
VENDOR_SRCS = vendor/sds/sds.c
OBJS = $(SRCS:src/%.c=build/%.o) \
		 $(VENDOR_SRCS:vendor/sds/sds.c=build/sds.o)

all: $(OBJS)
	$(LD) -o $(TARGET) $(OBJS) $(STATIC_LIBS) $(LDFLAGS)

build/%.o: src/%.c src/*.h $(VENDOR_SRCS)
	$(CC) -c $(CCFLAGS) $(INCLUDES) $< -o $@

build/sds.o: vendor/sds/sds.c
	$(CC) -c $(CCFLAGS) $(INCLUDES) $< -o $@


clean:
	rm -f build/*.o
