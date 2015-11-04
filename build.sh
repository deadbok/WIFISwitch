#!/bin/sh

if [ $# -gt 0 ]
then
make clean
fi

make all 2>&1 | tee build.log
cp ./build.log logs/build-`date +%Y-%m-%d-%H-%M-%S`.log

if [ $# -gt 1 ]
then
make debugflash
fi
