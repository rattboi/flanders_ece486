#!/bin/bash
#cat orig/fp.txt
#echo "FP: "
./predictor traces/DIST-FP-1

#cat orig/int.txt
#echo "IN: "
./predictor traces/DIST-INT-1

#cat orig/mm.txt
#echo "MM: "
./predictor traces/DIST-MM-1

#cat orig/serv.txt
#echo "SV: "
./predictor traces/DIST-SERV-1
printf '\a'
