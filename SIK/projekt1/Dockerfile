#!/usr/bin/env -S docker build . --tag ppcb:latest --compress --file

FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    iproute2 \
    net-tools \
    tcpdump \
    iputils-ping \
    strace \
    netcat-traditional \
    nmap \
    valgrind

COPY ppcbc /bin/ppcbc
COPY ppcbs /bin/ppcbs
COPY ppcat /bin/ppcat

CMD exec /bin/sh -c "trap : TERM INT; sleep infinity & wait"
