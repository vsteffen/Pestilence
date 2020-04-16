#!/bin/bash

SCRIPT=`realpath $0`
SCRIPT_PATH=`dirname $SCRIPT`

LOG=$SCRIPT_PATH/exec_all.log
rm -rf $LOG
touch $LOG

TEST_DIR="/tmp/test"
rm -rf $TEST_DIR
mkdir $TEST_DIR

cd $TEST_DIR
cp -rf /bin/* $TEST_DIR
cp -rf /sbin/* $TEST_DIR
cp -rf /usr/bin/* $TEST_DIR

timeout_sec="$1"
if [ -z "$1" ] ; then
	timeout_sec="1"
fi

$SCRIPT_PATH/../famine

for entry in $TEST_DIR/*
do
	# Removing programs which stop script
	if [[ "$entry" = "$TEST_DIR/pidof" \
	|| "$entry" = "$TEST_DIR/yes" \
	|| "$entry" = "$TEST_DIR/killall5" ]] ; then
		continue
	fi
	printf "\n------------------> $entry\n"
	if [ $? -eq 0 ] ; then
		echo $entry >> $LOG
		script -q -f -c "bash -c \"timeout --foreground --signal=SIGKILL $timeout_sec $entry 0>&-\"" -a $LOG | grep -v 'Script done' | grep -v 'Script started'
	fi
done

exit 0
