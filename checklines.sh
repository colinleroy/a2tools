#!/bin/bash

DUPS=$(cat $1 | cut -d' ' -f1 | sort | uniq -c | sort -n | grep "[0-9] [0-9]"|grep -v " 1 [0-9]"|sed "s/^.* //")

for d in $DUPS; do
  echo line $d is duplicated:
  grep ^$d $1
done

cat $1 | cut -d' ' -f1 | grep -v '^$' > /tmp/unsorted
cat $1 | cut -d' ' -f1 | grep -v '^$' | sort -n > /tmp/sorted

diff -u /tmp/unsorted /tmp/sorted
