#!/bin/bash

if [ "$1" = "" ]; then
  echo "Usage: $0 [directory]"
  exit 1
fi
ERR=0
for f in $1/*.s; do
  imports=$(grep '\.import' $f | sed "s/^.*\.importzp *//" | sed "s/^.*\.import *//" | sed "s/,/\\n/g")
  for i in $imports; do
    matches=$(grep -w $i $f|grep -v '\.import')
    if [ "$matches" = "" ]; then
      echo "$i is not used in $f"
      ERR=1
    fi
  done
done
exit $ERR
