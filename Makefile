CC=gcc
CFLAGS=-Wall -Werror -DLIN -DXPLM200 -DXPLM210

all: raberix.xpl

menu.o: menu.c menu.h
	$(CC) -c $(CFLAGS) -fPIC menu.c -o menu.o

main.o: main.c main.h menu.h
	$(CC) -c $(CFLAGS) -fPIC main.c -o main.o

raberix.xpl: main.o menu.o
	$(CC) -shared -o raberix.xpl main.o menu.o

.PHONY: clean
clean:
	rm -f *.o *.xpl
