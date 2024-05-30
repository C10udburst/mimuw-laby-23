#!/bin/bash
./kierki-serwer -f bruh.txt -t 5 &
PORT=$(lsof -P -n -i | grep kierki-se | awk '{print $9}' | awk -F ':' '{print $2}')
#./kierki-klient -h localhost -p $PORT -a -4 -N > /dev/null &
./kierki-klient -h localhost -p $PORT -a -4 -E > /dev/null &
./kierki-klient -h localhost -p $PORT -a -4 -S > /dev/null &
./kierki-klient -h localhost -p $PORT -a -4 -W > /dev/null &
read
killall kierki-serwer