#!/bin/bash

./run server tc qdisc add dev eth0 root netem loss 10%
./run client1 tc qdisc add dev eth0 root netem loss 5%
./run client2 tc qdisc add dev eth0 root netem loss 15%
./run client3 tc qdisc add dev eth0 root netem loss 20%