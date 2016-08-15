#!/bin/bash

#Run with sudo for the nice commands to work

#arg1 is the # of iterations of first size
#arg2 is the first size (in millions)

#arg3 is the # of iterations of second size
#arg4 is the second size (in millions)

#arg5 is the t

i=1
while [ $i -le "$1" ]
    do
    
    nice -n -10 ./PassCrack offline -v -q -g -p 4 \
    -o /media/tsaou/144E156E153D44/Users/Leo/Desktop/Tables/Table_$2Mx$5_pt$i.txt -m $2000000 -t $5
    
    i=$(($i+1))
    
    done
    
    
    
i=1    
while [ $i -le "$3" ]
    do
    
    nice -n -10 ./PassCrack offline -v -q -g -p 4 \
    -o /media/tsaou/144E156E153D44/Users/Leo/Desktop/Tables/Table_$4Mx$5_pt$i.txt -m $4000000 -t $5
    
    i=$(($i+1))
    
    done    
    
chown -v tsaou:tsaou /media/tsaou/144E156E153D44/Users/Leo/Desktop/Tables/Table*    
