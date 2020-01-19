CC=gcc
CFLAGS=-Wall -Werror -DLIN -DXPLM200 -DXPLM210
#CSOFLAGS=-fvisibility=hidden
PYTHON_VER=3
PYTHON_CFLAGS:=$(shell python$(PYTHON_VER)-config --cflags)
PYTHON_LDFLAGS:=$(shell python$(PYTHON_VER)-config --ldflags) -fno-lto

all: raberix.xpl

util.o: util.c util.h
	$(CC) -c $(CFLAGS) -fvisibility=hidden -fPIC $< -o $@

libhid.so: hid.o util.o
	$(CC) -shared $(CFLAGS) -fPIC $^ -o $@

hid.o: hid.c hid.h structure.h util.o def.h
	$(CC) -c $(CFLAGS) -fvisibility=hidden -fPIC $< -o $@

commandref.o: commandref.c commandref.h structure.h
	$(CC) -c $(CFLAGS) -fPIC $< -o $@

dataref.o: dataref.c dataref.h
	$(CC) -c $(CFLAGS) -fPIC $< -o $@

loop.o: loop.c loop.h
	$(CC) -c $(CFLAGS) -fPIC $< -o $@

pymodule.o: pymodule.c pymodule.h loop.h dataref.h commandref.h
	$(CC) -c $(CFLAGS) $(PYTHON_CFLAGS) -fPIC $< -o pymodule.o

menu.o: menu.c menu.h
	$(CC) -c $(CFLAGS) -fPIC $< -o $@

main.o: main.c pymodule.h menu.h loop.h dataref.h
	$(CC) -c $(CFLAGS) -fPIC $< -o $@

raberix.xpl: main.o menu.o pymodule.o loop.o dataref.o commandref.o util.o libhid.so
	$(CC) -Wl,-rpath,libhid.so -L. -l:libhid.so -shared -o $@ main.o menu.o pymodule.o loop.o dataref.o commandref.o util.o $(PYTHON_LDFLAGS)

.PHONY: clean
clean:
	rm -f *.o *.xpl *.so *-test

test.o: test.c test.h
	$(CC) -c $(CFLAGS) -fPIC $< -o $@

.PHONY: test
test: hid-test
	./hid-test

hid-test: hid-test.c test.o libhid.so util.o
	$(CC) $(CFLAGS) hid-test.c test.o util.o -Wl,-rpath,. -L. -l:libhid.so -o $@
