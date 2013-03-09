#!/bin/bash
echo Running DIST-FP-1
./predictor traces/DIST-FP-1
echo Running DIST-INT-1
./predictor traces/DIST-INT-1
echo Running DIST-MM-1
./predictor traces/DIST-MM-1
echo Running DIST-SERV-1
./predictor traces/DIST-SERV-1
printf '\a'
