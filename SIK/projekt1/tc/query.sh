#!/bin/bash

echo -n "server: "
./run server tc qdisc show dev eth0
echo -n "client1: "
./run client1 tc qdisc show dev eth0
echo -n "client2: "
./run client2 tc qdisc show dev eth0
echo -n "client3: "
./run client3 tc qdisc show dev eth0