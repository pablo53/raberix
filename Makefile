CC=gcc
CFLAGS=-Wall -Werror -DLIN -DXPLM200 -DXPLM210
PYTHON_VER=3
PYTHON_CFLAGS:=$(shell python$(PYTHON_VER)-config --cflags)
PYTHON_LDLAGS:=$(shell python$(PYTHON_VER)-config --ldflags) -fno-lto

all: raberix.xpl

commandref.o: commandref.c commandref.h structure.h
	$(CC) -c $(CFLAGS) -fPIC commandref.c -o commandref.o

dataref.o: dataref.c dataref.h
	$(CC) -c $(CFLAGS) -fPIC dataref.c -o dataref.o

loop.o: loop.c loop.h
	$(CC) -c $(CFLAGS) -fPIC loop.c -o loop.o

pymodule.o: pymodule.c pymodule.h loop.h dataref.h commandref.h
	$(CC) -c $(CFLAGS) $(PYTHON_CFLAGS) -fPIC pymodule.c -o pymodule.o

menu.o: menu.c menu.h
	$(CC) -c $(CFLAGS) -fPIC menu.c -o menu.o

main.o: main.c main.h pymodule.h menu.h loop.h dataref.h
	$(CC) -c $(CFLAGS) -fPIC main.c -o main.o

raberix.xpl: main.o menu.o pymodule.o loop.o dataref.o commandref.o
	$(CC) -shared -o raberix.xpl main.o menu.o pymodule.o loop.o dataref.o commandref.o $(PYTHON_LDLAGS)

.PHONY: clean
clean:
	rm -f *.o *.xpl
