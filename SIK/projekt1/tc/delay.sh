#!/bin/bash

./run server tc qdisc add dev eth0 root netem delay 10ms
./run client1 tc qdisc add dev eth0 root netem delay 7.5ms
./run client2 tc qdisc add dev eth0 root netem delay 15ms
./run client3 tc qdisc add dev eth0 root netem delay 20ms