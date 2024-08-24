#!/bin/bash
DESTDIR=$1
make clean
make -C surl-server prefix=/usr etc_prefix=/etc
make -C a2send a2send.x86_64 prefix=/usr etc_prefix=/etc
make -C a2recv a2recv.x86_64 prefix=/usr etc_prefix=/etc
make -C a2trace a2trace
