#!/bin/bash

./run server tc qdisc add dev eth0 root netem delay 10ms reorder 2.5%
./run client1 tc qdisc add dev eth0 root netem delay 7ms reorder 1%
./run client2 tc qdisc add dev eth0 root netem delay 150ms reorder 15%
./run client3 tc qdisc add dev eth0 root netem delay 200ms reorder 35%
