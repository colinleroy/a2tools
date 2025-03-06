#!/bin/bash

if [ "$1" = "" ]; then
  echo "Usage: $0 [template file] [list of level names]"
  exit 1
fi

TMPL=$1
shift

cat $TMPL | while IFS= read -r line ; do
  if [ "$line" = "### LEVEL_MEMORY_DECLARATIONS ###" ]; then
    for lvl in "$@"; do
      echo "    $lvl: file = \"$lvl.bin\",     start = __HGR_START__,     size = __LEVEL_SIZE__;"
    done
  elif [ "$line" = "### LEVEL_SEGMENTS_DECLARATIONS ###" ]; then
    for lvl in "$@"; do
      echo "    $lvl:   load = $lvl,       type = ro;"
    done
  else
    echo "$line"
  fi
done
