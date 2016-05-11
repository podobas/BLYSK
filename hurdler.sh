#!/bin/bash -ex

export PATH="$PWD/shell/:$PATH"
. $PWD/shell/base.sh
#export RESULTS="$PWD/../results"
#~ threads=$(seq 2)
threads=$(seq 1 8)
function run() {
	for t in $threads
	do
		export OMP_NUM_THREADS=$t
		for ITT in $(seq 30)
		do
			export ITT
			c.sh ./Hurdler 1000000 10000 >> Hurdler.$ENABLE_SPEC.tsv
		done
		#~ p.sh ./Hurdler 1000000 10000
		#~ i.sh ./Hurdler >> Hurdler.$ENABLE_SPEC.tsv
	done
}

function buildRun() {
	unset LIBS
	make clean
	make -j
	. env.sh
	cd Hurdler
	make
	run
	cd ..
}

echo "-------ENABLED SPEC"
export ENABLE_SPEC="1"
buildRun

echo "-------DISABLED SPEC"
export ENABLE_SPEC=""
buildRun


