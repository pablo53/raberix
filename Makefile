CC=gcc
CFLAGS=-Wall -Werror -DLIN -DXPLM200 -DXPLM210
PYTHON_VER=3.7
PYTHON_CFLAGS:=$(shell python$(PYTHON_VER)-config --cflags)
PYTHON_LDLAGS:=$(shell python$(PYTHON_VER)-config --ldflags) -fno-lto

all: raberix.xpl

pymodule.o: pymodule.c pymodule.h
	$(CC) -c $(CFLAGS) $(PYTHON_CFLAGS) -fPIC pymodule.c -o pymodule.o

menu.o: menu.c menu.h
	$(CC) -c $(CFLAGS) -fPIC menu.c -o menu.o

main.o: main.c main.h menu.h
	$(CC) -c $(CFLAGS) -fPIC main.c -o main.o

raberix.xpl: main.o menu.o pymodule.o
	$(CC) -shared -o raberix.xpl main.o menu.o pymodule.o $(PYTHON_LDLAGS)

.PHONY: clean
clean:
	rm -f *.o *.xpl
