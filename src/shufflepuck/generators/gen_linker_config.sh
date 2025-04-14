#!/bin/bash

if [ "$1" = "" ]; then
  echo "Usage: $0 [template file] [list of assets]"
  exit 1
fi

TMPL=$1
shift

cat $TMPL | while IFS= read -r line ; do
  if [ "$line" = "### LEVEL_MEMORY_DECLARATIONS ###" ]; then
    for lvl in "$@"; do
      echo "    $lvl: file = \"opponent_$lvl.bin\",     start = __OPPONENT_START__,     size = __OPPONENT_SIZE__;"
    done
  elif [ "$line" = "### LEVEL_SEGMENTS_DECLARATIONS ###" ]; then
    for lvl in "$@"; do
      echo "    $lvl:   load = $lvl,       type = ro, align = \$100;"
    done
  else
    echo "$line"
  fi
done
