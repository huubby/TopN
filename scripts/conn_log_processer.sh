#!/bin/bash

if [ $# != 1 ]; then
	echo "Usage: $0 DATE[2013-05-14]"
	exit
fi

DATE=$1

# Decompression
./decompress.sh $DATE
if [ ! $? == 0 ]; then
	exit
fi

# Analysis
pushd $DATE > /dev/null

if [ -f $DATE.log ]; then
	../analyzer -f $DATE.log -a ../a.list > /dev/null
	../combiner c.list > combined.list

	# Generate routes from combined list
	../routes.sh $DATE
	rm $DATE.log
fi

popd >> /dev/null
