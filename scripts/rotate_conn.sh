CWD=`pwd`
PID=`/sbin/pidof conntrack`
HOUR=`date +%F-%H`
FINAL=$CWD/$HOUR.log
LOG=$CWD/conntrack.log

kill $PID
mv $LOG $FINAL
/usr/local/sbin/conntrack -E -e DESTROY | grep ASSURED > $LOG
gzip $FINAL
