#!/bin/bash

START_MONTH=5
START_DAY=14
END_MONTH=`date +%-m`
END_DAY=`date +%-d`

DAYS_IN_MONTH=(31 28 31 30 31 30 31 31 30 31 30 31)

CWD=`pwd`
let COUNT=0
let DAYS=0
let FIRST=1
for ((i=$START_MONTH; i<=$END_MONTH; i++)); do
    for ((j=1; j<${DAYS_IN_MONTH[$i]}; j++)); do
	if [ $i == '5' -a $FIRST == 1 ]; then
	    j=$START_DAY
	    FIRST=0
	fi
	DATE=2013-`printf "%02d" $i`-`printf "%02d" $j`
	./decompress.sh $DATE
	if [ ! $? == 0 ]; then
	    exit
	fi

	SUM=0
	cd $DATE
	if [ -f $DATE.log ]; then
	    ../analyzer -f $DATE.log -a ../a.list > /dev/null
	    ../combiner c.list > combined.list
	    rm $DATE.log
	fi

	ROUTE_NUM=10
	if [ -f combined.list ]; then
	    ROUTE_NUM=`wc -l combined.list|cut -f1 -d " "`
	fi

	echo $DATE, route count: $ROUTE_NUM
	COUNT=$(expr $COUNT + $ROUTE_NUM)
	let DAYS++

	cd $CWD
	if [ $i == $END_MONTH -a $j == $END_DAY ]; then
	    break
	fi
    done
done

echo Average: $(($COUNT / $DAYS)) routes per day.
