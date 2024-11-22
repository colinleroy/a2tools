#!/bin/bash
export LC_ALL=en_US.UTF-8
export LANG=en_US

tags=$(git tag)
tags=$(echo "$tags master")
echo "Version,Main program,Composer,Image viewer,Login handler,Configurator" > sizes.csv
echo "string,number,number,number,number,number" >> sizes.csv
for t in $tags; do
  rm -f ../../socket.localhost:200* ../quicktake/splash.h
  git checkout $t || exit
  make clean all || exit
  d=$(git log --date='format:%b %Y'|grep ^Date|sed "s/^Date:...//"|head -1)
  cli=$((du -b mastocli.bin || echo 0)|sed 's/\t.*$//')
  write=$((du -b mastowrite.bin || echo 0)|sed 's/\t.*$//')
  img=$((du -b mastoimg.bin || echo 0)|sed 's/\t.*$//')
  don=$((du -b mastodon.bin || echo 0)|sed 's/\t.*$//')
  conf=$((du -b mastoconf.bin || echo 0)|sed 's/\t.*$//')
  
  echo "$d ($t),$cli,$write,$don,$conf,$img" >> sizes.csv
done
