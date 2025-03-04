#!/bin/bash

echo '.segment "HGR"'
echo '_splash_hgr:'
cat $1 | od -t x1 -v -An | \
  sed 's/^ /$/' | sed 's/ /, $/g' | sed 's/$/,/' | \
  sed 's/^/         .byte /' | sed 's/,$//'
