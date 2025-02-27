#!/bin/sh

GIMP_CONFIG=~/.config/GIMP/
DEST_DIRS=$(ls $GIMP_CONFIG)
for d in $DEST_DIRS; do
  ln -s $(pwd)/assets/patterns/*.png $GIMP_CONFIG/$DEST_DIRS/patterns/
done
