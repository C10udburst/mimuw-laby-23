#!/bin/bash

./run server tc qdisc add dev eth0 root netem delay 100ms
./run client1 tc qdisc add dev eth0 root netem delay 75ms
./run client2 tc qdisc add dev eth0 root netem delay 150ms
./run client3 tc qdisc add dev eth0 root netem delay 200ms