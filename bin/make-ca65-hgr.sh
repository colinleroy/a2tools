#!/bin/bash
NAME=$(echo $1|sed "s/\.hgr//")
if [ "$2" != "" ]; then
  SEG=$NAME
else
  SEG=HGR
fi
echo ".segment \"$SEG\""
echo ".assert * = \$2000, error ; Segment must start with the HGR data"
echo "_${NAME}_hgr:"
cat $1 | od -t x1 -v -An | \
  sed 's/^ /$/' | sed 's/ /, $/g' | sed 's/$/,/' | \
  sed 's/^/         .byte /' | sed 's/,$//'
