#!/bin/bash

./run server tc qdisc add dev eth0 root netem delay 100ms reorder 25% 50%
./run client1 tc qdisc add dev eth0 root netem delay 75ms reorder 10% 50%
./run client2 tc qdisc add dev eth0 root netem delay 150ms reorder 15% 25%
./run client3 tc qdisc add dev eth0 root netem delay 200ms reorder 35% 15%
