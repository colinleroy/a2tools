#!/bin/bash

if [ "$1" = "" ]; then
  echo "Usage: $0 [directory1] [directory2]"
  exit 1
fi
ERR=0
for f in $1/*.s $2/*.s *.gen.s; do
  exports=$(grep '\.export' $f | sed "s/^.*\.export *//" | sed "s/,/\\n/g")
  for i in $exports; do
    matches=$(grep -w $i $1/*.s $2/*.s|grep -v '\.export')
    if [ "$matches" = "" ]; then
      echo "$i is not used in $f"
      ERR=1
    fi
  done
done
exit $ERR
