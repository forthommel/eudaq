#!/bin/sh
BINPATH=../../../bin
$BINPATH/euLog&
sleep 1
$BINPATH/euCliMonitor -n SrsMonitor -t srs_mon &
$BINPATH/euCliCollector -n SrsDirectSaveDataCollector -t srs_dc &
$BINPATH/euCliProducer -n SrsProducer -t srs_pd &
