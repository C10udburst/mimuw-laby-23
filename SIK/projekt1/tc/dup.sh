#!/bin/bash

./run server tc qdisc add dev eth0 root netem duplicate 5%
./run client1 tc qdisc add dev eth0 root netem duplicate 5%
./run client2 tc qdisc add dev eth0 root netem duplicate 7%
./run client3 tc qdisc add dev eth0 root netem duplicate 10%