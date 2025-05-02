#!/bin/sh

DAEMON="/opt/ElectronicTeam/eveusb/bin/eveusbd"
OPTIONS="--moduless --loglevel=notice"
LOGFILE="/var/log/eveusbd_watchdog.log"

mkdir -p /var/log

while true
do
    if ! pgrep -x "eveusbd" > /dev/null
    then
        echo "$(date '+%Y-%m-%d %H:%M:%S') eveusbd not running. Restarting..." >> "$LOGFILE"
        $DAEMON $OPTIONS &
        sleep 2
    fi
    sleep 5
done

