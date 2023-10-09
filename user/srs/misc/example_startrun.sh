#!/bin/sh
#export LD_LIBRARY_PATH=~/srsdriver/build/:$LD_LIBRARY_PATH

BINPATH=../../../bin
$BINPATH/euLog&
sleep 1
$BINPATH/euCliMonitor -n SrsMonitor -t srs_mon &
$BINPATH/euCliCollector -n SrsDirectSaveDataCollector -t srs_dc &
$BINPATH/euCliProducer -n SrsProducer -t srs_pd &
