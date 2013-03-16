#!/bin/bash
cat orig/fp.txt
echo -e "\nNew:"
./predictor traces/DIST-FP-1

cat orig/int.txt
echo -e "\n"New:
./predictor traces/DIST-INT-1

cat orig/mm.txt
echo -e "\n"New:
./predictor traces/DIST-MM-1

cat orig/serv.txt
echo -e "\n"New:
./predictor traces/DIST-SERV-1
printf '\a'
