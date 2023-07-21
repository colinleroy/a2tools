#!/bin/bash
DESTDIR=$1
make clean
make -C surl-server
make -C a2send a2send.x86_64
make -C a2recv a2recv.x86_64
