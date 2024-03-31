.PHONY: all clean

CC = g++
CFLAGS = -Wall -Wextra -std=c++23 -g

all: ppcbs ppcbc

clean:
	rm -f *.o
	rm -f *.out
	rm -f *.tar
	rm -f *.tar.gz
	rm -f ppcbs ppcbc

packets.o: packets.cpp packets.h
	$(CC) $(CFLAGS) -c packets.cpp

common.o: common.cpp common.h
	$(CC) $(CFLAGS) -c common.cpp

server.o: server.cpp packets.h common.h
	$(CC) $(CFLAGS) -c server.cpp

client.o: client.cpp packets.h common.h
	$(CC) $(CFLAGS) -c client.cpp

cat.o: cat.cpp packets.h common.h
	$(CC) $(CFLAGS) -c cat.cpp

pnc: cat.o packets.o common.o
	$(CC) $(CFLAGS) -o pnc $^

ppcbs: server.o packets.o common.o
	$(CC) $(CFLAGS) -o ppcbs $^

ppcbc: client.o packets.o common.o
	$(CC) $(CFLAGS) -o ppcbc $^