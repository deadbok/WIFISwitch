#!/bin/sh

DEBUG=1 DEBUGFS=1 make $* 2>&1 | tee build.log
cp ./build.log logs/build-`date +%Y-%m-%d-%H-%M-%S`.log
