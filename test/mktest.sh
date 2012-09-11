#! /bin/bash

i="0"; 

while [ $i -lt 10000 ]; do 
    cat nat.log >> nat.log.test 
    i=$[$i+1]; 
done
