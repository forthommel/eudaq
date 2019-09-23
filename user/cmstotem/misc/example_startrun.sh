#!/usr/bin/env sh
BINPATH=../../../bin
#$BINPATH/euRun &
#sleep 1
$BINPATH/euLog &
sleep 1
$BINPATH/euCliMonitor -n SampicMonitor -t sampic_mon &
$BINPATH/euCliCollector -n SampicDirectSaveDataCollector -t sampic_dc &
$BINPATH/euCliProducer -n SampicProducer -t sampic_pd &
#$BINPATH/euCliConverter -n SampicRaw -t sampic_
