#!/bin/bash

ModelBoardSize=$1
COUNT=$2
NF=$3
START=$4 
END=$5
GAP=$6

FOLDER_NAME="slither$ModelBoardSize"

cd storage/model
mkdir -p ./manual
mkdir -p ./manual/$FOLDER_NAME

for (( i=$START+$GAP; i <= $END; i+=$GAP ))
do
    FILE1=$(printf "%05d" $(( i )) );
    FILE2=$(printf "%05d" $(( (i-$GAP) )) );
    eval "python3 -m clap.tool.CLI_agent -game slither -model $FOLDER_NAME/$FILE1.pt -opp_model $FOLDER_NAME/$FILE2.pt -count $COUNT -mode fight -nf $NF"
done
