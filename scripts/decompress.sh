#!/bin/bash

DATE=$1
SUFFIX=gz

if [ ! -d $DATE ]; then
    mkdir $DATE
fi

for i in `seq 0 23`; do 
    H=`printf "%02d" $i` 
    FILE=$DATE-$H.log.$SUFFIX

    if [ -f ../$FILE ]; then
	pushd $DATE > /dev/null
    	gunzip -c ../../$FILE > $DATE-$H.log
	cat $DATE-$H.log >> $DATE.log
	if [ $? == 0 ]; then
	    rm $DATE-$H.log
	fi
	popd > /dev/null
    fi
done;
