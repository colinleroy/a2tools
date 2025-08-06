#!/bin/bash

i=0
while true; do
  i=$(($i+1))
  GOOD=$(head -$i $1|tail -1|cut -d' ' -f$3)
  BAD=$(head -$i $2|tail -1|cut -d' ' -f$3)
  if [ "$GOOD" != "$BAD" ]; then
    echo "Read diff at line $i: $GOOD vs $BAD"
  fi
done
