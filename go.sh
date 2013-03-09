#!/bin/bash
echo Old DIST-FP-1
cat orig/fp.txt
echo New:
./predictor traces/DIST-FP-1

echo Old DIST-INT-1
cat orig/int.txt
echo New:
./predictor traces/DIST-INT-1

echo Old DIST-MM-1
cat orig/mm.txt
echo New:
./predictor traces/DIST-MM-1

echo Old DIST-SERV-1
cat orig/serv.txt
echo New:
./predictor traces/DIST-SERV-1
printf '\a'
