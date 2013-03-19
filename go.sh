#!/bin/bash
echo "FP Tests:"
./predictor traces/DIST-FP-1
./predictor traces/DIST-FP-2
./predictor traces/DIST-FP-3
./predictor traces/DIST-FP-4
./predictor traces/DIST-FP-5

echo "INT Tests:"
./predictor traces/DIST-INT-1
./predictor traces/DIST-INT-2
./predictor traces/DIST-INT-3
./predictor traces/DIST-INT-4
./predictor traces/DIST-INT-5

echo "MM Tests:"
./predictor traces/DIST-MM-1
./predictor traces/DIST-MM-2
./predictor traces/DIST-MM-3
./predictor traces/DIST-MM-4
./predictor traces/DIST-MM-5

echo "SERV Tests:"
./predictor traces/DIST-SERV-1
./predictor traces/DIST-SERV-2
./predictor traces/DIST-SERV-3
./predictor traces/DIST-SERV-4
./predictor traces/DIST-SERV-5
printf '\a'
