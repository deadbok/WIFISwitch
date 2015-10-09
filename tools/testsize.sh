#!/bin/bash
img_size=$(stat -c %s $1)
if [ $img_size -gt $2 ]
then 
  echo "Image too large";
  exit 1; 
fi 
