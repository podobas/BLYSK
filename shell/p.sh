#!/bin/bash -e
. $(dirname $0)/base.sh

eStr=""
for e in $(echo $events | sed 's! seconds!!')
do
	eStr="$eStr -e $e"
done
# -e L1-dcache-load-misses -e L1-dcache-store-misses -e cache-misses -e cycles -e instructions -e cpu-clock -e task-clock -e page-faults -e minor-faults
perf stat $eStr "$@"
