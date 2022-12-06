#!/bin/bash

if [ "$1" = "" ]; then
  echo "Usage: $0 [file]"
  exit 1
fi

FNAME=$(basename $1 | sed "s/\..*/")
LEN=$(wc -c $1)

echo $FNAME
sleep 1
echo $LEN
sleep 1

ascii-xfr -nsv -l 15 -c 15 $1 > /dev/ttyUSB0
