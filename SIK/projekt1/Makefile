CC = g++
CFLAGS = -Wall -Wextra -std=c++23 -g -O0

all: ppcbs ppcbc ppcat

clean:
	docker compose down || true
	docker compose rm -f || true
	rm -f *.o ppcbs ppcbc ppcat
	rm ppcb.tgz

ppcb.tgz: Makefile.release common.cpp common.h server.cpp server.h client.cpp client.h ppcbs.cpp ppcbc.cpp ppcat.cpp protconst.h exceptions.h
	tar --transform 'flags=r;s|Makefile.release|Makefile|' -czf $@ $^

common.o: common.cpp common.h protconst.h exceptions.h
	$(CC) $(CFLAGS) -c common.cpp -o $@

server.o: server.cpp server.h common.h
	$(CC) $(CFLAGS) -c server.cpp -o $@

client.o: client.cpp client.h common.h
	$(CC) $(CFLAGS) -c client.cpp -o $@

ppcbs.o: ppcbs.cpp server.h common.h
	$(CC) $(CFLAGS) -c ppcbs.cpp -o $@

ppcbc.o: ppcbc.cpp client.h common.h
	$(CC) $(CFLAGS) -c ppcbc.cpp -o $@

ppcat.o: ppcat.cpp client.h server.h common.h
	$(CC) $(CFLAGS) -c ppcat.cpp -o $@

ppcbs: common.o server.o ppcbs.o
	$(CC) $(CFLAGS) common.o server.o ppcbs.o -o $@

ppcbc: common.o client.o ppcbc.o
	$(CC) $(CFLAGS) common.o client.o ppcbc.o -o $@

ppcat: common.o client.o server.o ppcat.o
	$(CC) $(CFLAGS) common.o client.o server.o ppcat.o -o $@

docker: Dockerfile docker-compose.yml ppcat ppcbc ppcbs
	docker compose up --build -d

.PHONY: all clean