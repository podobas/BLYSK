#!/bin/bash

set -xe

if [ x$PREFIX = x ]
then
	export PREFIX=$PWD/../install
	rm -rf $PREFIX
	mkdir -p "$PREFIX/bin"
	mkdir -p "$PREFIX/usr/lib"
	mkdir -p "$PREFIX/usr/include"
fi

## Build blysk and libpappadapt
make clean
make -j 4
make install
make -C ../papp_adaptivity_prototype clean
make -C ../papp_adaptivity_prototype -j 4
make -C ../papp_adaptivity_prototype install

## Build the application with the libraries
export LD_LIBRARY_PATH="$PREFIX/usr/lib"
export LIBS="-L$LD_LIBRARY_PATH"
make -C ../tests/UC_RadarDet clean
make -C ../tests/UC_RadarDet

exit 0

## Run the application with sudo
sudo LD_LIBRARY_PATH="$LD_LIBRARY_PATH" bash << EOF
cd ../tests/UC_RadarDet/apps

## Set the system adaptivity parameters (you should update these)
export PAPP_SYS_MIN_TEMP=55000
export PAPP_SYS_MAX_TEMP=72000
export PAPP_SYS_TARGET_TEMP=65000
export PAPP_SYS_FREQ=10
export OMP_TASK_SCHEDULER=worksteal
export OMP_TASK_ADAPTATION=libpappadapt.so

## Run the actual applicaiton, should use bigger input
./radar_sigproc ../input_large.csv output.csv
EOF
