#!/usr/bin/env sh
BINPATH=../../../bin
#$BINPATH/euRun &
#$BINPATH/euRun -n SampicRunControl &
#sleep 1
$BINPATH/euLog &
sleep 1
$BINPATH/euCliMonitor -n SampicMonitor -t sampic_mon &
###$BINPATH/euCliCollector -n SampicDataCollector -t sampic_dc &
# The following data collectors are provided if you build user/eudet
$BINPATH/euCliCollector -n SampicDirectSaveDataCollector -t sampic_dc &
#$BINPATH/euCliCollector -n EventIDSyncDataCollector -t sampic_dc &
#$BINPATH/euCliCollector -n TriggerIDSyncDataCollector -t sampic_dc &
$BINPATH/euCliProducer -n SampicProducer -t sampic_pd &
#$BINPATH/euCliCollector -n EventIDSyncDataCollector -t sampic_dc &
#$BINPATH/euCliConverter -n SampicRaw -t sampic_
