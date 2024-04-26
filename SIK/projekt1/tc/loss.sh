#!/bin/bash

./run server tc qdisc add dev eth0 root netem loss 2%
./run client1 tc qdisc add dev eth0 root netem loss 1%
./run client2 tc qdisc add dev eth0 root netem loss 7%
./run client3 tc qdisc add dev eth0 root netem loss 10%