CC = g++
CFLAGS = -Wall -Wextra -std=c++20 -g -O0 -pthread

all: kierki-serwer kierki-klient

clean:
	rm -f *.o kierki-serwer kierki-klient kierki.tgz

kierki.tgz: Makefile.release common.cpp common.h kierki-serwer.cpp kierki-klient.cpp
	tar --transform 'flags=r;s|Makefile.release|Makefile|' -czf $@ $^

wireshark: kierki.lua
	nohup wireshark -X lua_script:kierki.lua 2>/dev/null &

common.o: common.cpp common.h
	$(CC) $(CFLAGS) -c -o $@ $<

kierki-serwer.o: kierki-serwer.cpp common.h
	$(CC) $(CFLAGS) -c -o $@ $<

kierki-klient.o: kierki-klient.cpp common.h
	$(CC) $(CFLAGS) -c -o $@ $<

kierki-serwer: kierki-serwer.o common.o

kierki-klient: kierki-klient.o common.o