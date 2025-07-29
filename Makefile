TARGET=bin/main
CC=gcc
LD=gcc
DEBUG=-g
OPT=-O0
WARN=-Wall
PTHREAD=-pthread
CCFLAGS=$(DEBUG) $(OPT) $(WARN) $(PTHREAD) -pipe
INCLUDES=-I./src/ -I./vendor/raylib/include/ -I./vendor/fftw/api
LIBS=-lGL -lm -lpthread -ldl -lrt -lX11
STATIC_LIBS=./vendor/raylib/lib/libraylib.a ./vendor/fftw/.libs/libfftw3.a
LDFLAGS=-export-dynamic $(PTHREAD) $(INCLUDES) $(LIBS)

OBJS=build/main.o

all: $(OBJS)
	$(LD) -o $(TARGET) $(OBJS) $(STATIC_LIBS) $(LDFLAGS)

build/main.o: src/main.c
	$(CC) -c $(CCFLAGS) $(INCLUDES) src/main.c -o build/main.o

clean:
	rm -f build/*.o
