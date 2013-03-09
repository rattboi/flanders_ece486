#!/bin/bash
echo Old DIST-FP-1
cat orig/fp.txt
echo \nNew:
./predictor traces/DIST-FP-1

echo \nOld DIST-INT-1
cat orig/int.txt
echo \nNew:
./predictor traces/DIST-INT-1

echo \nOld DIST-MM-1
cat orig/mm.txt
echo \nNew:
./predictor traces/DIST-MM-1

echo \nOld DIST-SERV-1
cat orig/serv.txt
echo \nNew:
./predictor traces/DIST-SERV-1
printf '\a'
