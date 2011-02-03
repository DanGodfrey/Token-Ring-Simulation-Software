#!/bin/sh
#set -x

# Shell script for testing Assignment 1

PSARG=-Hm
CUTARG="-f 2"

make >/tmp/make$$.log 2>&1 #Compile
if [ $? -ne 0 ]    
then
    # Compilation unsuccessful
    echo Compilation unsuccessful
else
    # Compilation successful - run the application
    echo Running hub
    hub >/tmp/hub$$.log 2>&1 &    # Put into the background
    sleep 1   # wait until all is started
    #List the processes and their hierarchy
    echo --------------------- Processes ----------------------- >tstlog.txt
    ps $PSARG >>tstlog.txt 2>&1
    echo ------------------------------------------------------- >>tstlog.txt
    # Get the pids for the hub process
    HUBPID=`fgrep -m 1 hub <tstlog.txt | cut -d ' ' $CUTARG`
    # Get the pids for the station process
    STATIONAPID=`fgrep -m 1 stn <tstlog.txt | cut -d ' ' $CUTARG`
    STATIONBPID=`fgrep -m 2 stn <tstlog.txt | cut -d ' ' $CUTARG`
    STATIONBPID=`echo $STATIONBPID | cut -d ' ' -f 2`
    STATIONCPID=`fgrep -m 3 stn <tstlog.txt | cut -d ' ' $CUTARG`
    STATIONCPID=`echo $STATIONCPID | cut -d ' ' -f 3`
    STATIONDPID=`fgrep -m 4 stn <tstlog.txt | cut -d ' ' $CUTARG`
    STATIONDPID=`echo $STATIONDPID | cut -d ' ' -f 4`
    # Find the pipes for all processes
    echo ----------------------- Pipes -------------------------- >>tstlog.txt
    echo Hub Process \($HUBPID\) >>tstlog.txt 2>&1
    ls -l /proc/$HUBPID/fd >>tstlog.txt 2>&1
    echo Station Process \($STATIONAPID\) >>tstlog.txt 2>&1
    ls -l /proc/$STATIONAPID/fd >>tstlog.txt 2>&1
    echo Station Process \($STATIONBPID\) >>tstlog.txt 2>&1
    ls -l /proc/$STATIONBPID/fd >>tstlog.txt 2>&1
    echo Station Process \($STATIONCPID\) >>tstlog.txt 2>&1
    ls -l /proc/$STATIONCPID/fd >>tstlog.txt 2>&1
    echo Station Process \($STATIONDPID\) >>tstlog.txt 2>&1
    ls -l /proc/$STATIONDPID/fd >>tstlog.txt 2>&1
    echo ------------------------------------------------------- >>tstlog.txt
    sleep 7  # wait 35 seconds until all is done
    # Add exchange to the end of the tstlog.txt file
    echo ----------------------- /tmp/hub.log ------------------- >>tstlog.txt
    cat /tmp/hub$$.log >>tstlog.txt
    echo ------------------------------------------------------- >>tstlog.txt
    echo ----------------------- /tmp/make.log ------------------- >>tstlog.txt
    cat /tmp/make$$.log >>tstlog.txt
fi
rm /tmp/hub$$.log /tmp/make$$.log
