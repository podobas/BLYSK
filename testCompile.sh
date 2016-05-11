#!/bin/bash
if [ x$PAPP_PATH = x ]
then
	echo "Must set PAPP_PATH to the root of the PaPP tools"
	exit 1
fi

#~ svn co https://svn.sics.se/PaPP/Code/WP4/papp_adaptivity_prototype ../papp_adaptivity_prototype

set -xe
#~ make distclean

#~ PATH="$PAPP_PATH:$PATH"

path=$PATH
platforms="$(echo $PAPP_PATH/*unknown-linux-* | sed "s!$PAPP_PATH!!g" | sed 's!/!!g')"
for p in $platforms
do
	echo "Installing BLYSK for $p"
	export CROSS="$p-"
	export TOOLS_APPS_PREFIX="$p"
	export PATH=$PAPP_PATH/$p/bin/:$path
	export PREFIX=$PAPP_PATH/$p/sys-root/
	make clean
	make
	make install
	make -C ../papp_adaptivity_prototype clean
	make -C ../papp_adaptivity_prototype
	make -C ../papp_adaptivity_prototype install
	make -C ../tests/UC_RadarDet/ clean
	make -C ../tests/UC_RadarDet/
done
#~ echo $platforms

#~ CROSS=
