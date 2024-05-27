CC = g++
CFLAGS = -Wall -Wextra -std=c++20 -g -O0 -pthread

all: kierki-serwer kierki-klient

clean:
	rm -f *.o */*.o kierki-serwer kierki-klient kierki.tgz

kierki.tgz: Makefile.release *.cpp *.h */*.cpp */*.h
	tar --transform 'flags=r;s|Makefile.release|Makefile|' -czf $@ $^

wireshark: kierki.lua
	nohup wireshark -X lua_script:kierki.lua >/dev/null 2>&1 &

%.o: %.cpp %.h
	$(CC) $(CFLAGS) -c $< -o $@

kierki-%.o: kierki-%.cpp common.h
	$(CC) $(CFLAGS) -c $< -o $@

common.o: common/card.o common/utils.o common/connection.o common/rules.o
	ld -r $^ -o $@

server.o: server/client.o server/gamefile.o server/sync.o server/server.o
	ld -r $^ -o $@

client.o: client/deal.o
	ld -r $^ -o $@

kierki-serwer: kierki-serwer.o common.o server.o
	$(CC) $(CFLAGS) $^ -o $@

kierki-klient: kierki-klient.o common.o client.o
	$(CC) $(CFLAGS) $^ -o $@