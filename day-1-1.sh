#!/bin/bash

MAX_SUM=0
CUR_SUM=0

while read -r line; do
  if [ "$line" != "" ]; then
    CUR_SUM=$(($CUR_SUM + $line))
  else
    if [ $CUR_SUM -gt $MAX_SUM ]; then
      MAX_SUM=$CUR_SUM
    fi
    CUR_SUM=0
  fi
done

echo max: $MAX_SUM

