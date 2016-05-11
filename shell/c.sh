#!/bin/bash -ex

. $(dirname $0)/base.sh

# Measure energy consumed before
p="/sys/devices/virtual/powercap/intel-rapl/intel-rapl:0"
if [ -e "$p/energy_uj" ]
then
	b0="$(cat $p/energy_uj)"
else
	b0="0"
fi
if [ -e "$p/intel-rapl:0:0/energy_uj" ]
then
	b00="$(cat $p/intel-rapl:0:0/energy_uj)"
else
	b00="0"
fi
if [ -e "$p/intel-rapl:0:1/energy_uj" ]
then
	b01="$(cat $p/intel-rapl:0:1/energy_uj)"
else
	b01="0"
fi

# Run
p.sh "$@" >& out.log

# Measure energy consumed after
if [ -e "$p/energy_uj" ]
then
	a0="$(cat $p/energy_uj)"
else
	a0="0"
fi
if [ -e "$p/intel-rapl:0:0/energy_uj" ]
then
	a00="$(cat $p/intel-rapl:0:0/energy_uj)"
else
	a00="0"
fi
if [ -e "$p/intel-rapl:0:1/energy_uj" ]
then
	a01="$(cat $p/intel-rapl:0:1/energy_uj)"
else
	a01="0"
fi

o "$*" $events
echo -e "\t$(($a0 - $b0))\t$(($a00 - $b00))\t$(($a01 - $b01))\t$OMP_NUM_THREADS\t$ENABLE_SPEC\t$ITT"

