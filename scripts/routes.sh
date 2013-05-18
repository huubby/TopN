#!/bin/bash

export PATH=$PATH:/sbin

FILE=$1
TABLE=10
NEXT_HOP=`ip route | grep default | cut -f3 -d" "`
DEV=eth2

# Insert destination IP address to an extra route table
while read ip 
do
    ip route add table $TABLE $ip nexthop via $NEXT_HOP  #dev $DEV #
    if [ $? != "0" ]; then
        echo Failed to insert $ip into route table $TABLE
    fi
done < $FILE

# Redirect traffic to monitoring interface 
ip route replace table $TABLE 192.168.1.0/24 dev eth0
# Add default route
ip route add table $TABLE default nexthop via $NEXT_HOP
