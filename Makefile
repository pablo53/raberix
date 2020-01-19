CC=gcc
CFLAGS=-Wall -Werror -DLIN -DXPLM200 -DXPLM210
PYTHON_VER=3
PYTHON_CFLAGS:=$(shell python$(PYTHON_VER)-config --cflags)
PYTHON_LDLAGS:=$(shell python$(PYTHON_VER)-config --ldflags) -fno-lto

all: raberix.xpl

hid.o: hid.c hid.h
	$(CC) -c $(CFLAGS) -fPIC $< -o hid.o

commandref.o: commandref.c commandref.h structure.h
	$(CC) -c $(CFLAGS) -fPIC $< -o commandref.o

dataref.o: dataref.c dataref.h
	$(CC) -c $(CFLAGS) -fPIC $< -o dataref.o

loop.o: loop.c loop.h
	$(CC) -c $(CFLAGS) -fPIC $< -o loop.o

pymodule.o: pymodule.c pymodule.h loop.h dataref.h commandref.h
	$(CC) -c $(CFLAGS) $(PYTHON_CFLAGS) -fPIC $< -o pymodule.o

menu.o: menu.c menu.h
	$(CC) -c $(CFLAGS) -fPIC $< -o menu.o

main.o: main.c main.h pymodule.h menu.h loop.h dataref.h
	$(CC) -c $(CFLAGS) -fPIC $< -o main.o

raberix.xpl: main.o menu.o pymodule.o loop.o dataref.o commandref.o hid.o
	$(CC) -shared -o raberix.xpl $^ $(PYTHON_LDLAGS)

.PHONY: clean
clean:
	rm -f *.o *.xpl *-test

test.o: test.h test.c
	$(CC) -c $(CFLAGS) -fPIC test.c -o test.o

.PHONY: test
test: hid-test
	./hid-test

hid-test: testhid.c test.o hid.o
	$(CC) $(CFLAGS) $^ -o $@
