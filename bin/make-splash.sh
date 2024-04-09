#!/bin/bash
if [ "$2" = "" ]; then
  OUT="splash.h"
else
  OUT=$2
fi

(
echo '#ifndef __splash_h'
echo '#define __splash_h'
echo
echo '#include "platform.h"'
echo
echo '#pragma data-name(push,"HGR")'
echo 'uint8 splash_hgr[] = {'
cat $1.hgr | od -t x1 -v -An | sed 's/^ /0x/' | sed 's/ /, 0x/g' | sed 's/$/,/'
echo '};'
echo '#pragma data-name(pop)'
echo
echo '#endif'
) > $OUT
