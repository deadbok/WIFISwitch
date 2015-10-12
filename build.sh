#!/bin/sh

make all 2>&1 | tee build.log
cp ./build.log logs/build-`date +%Y-%m-%d-%H-%M-%S`.log
