#!/bin/bash -ex

function run() {
	# Clean
	
	pushd ..
	make -C turboblysk clean
	make -C bots clean
	make -C backoff clean
	rm -f bots/bin/*
	echo tests/* | xargs -n 1 make clean -C 
	popd
	
	
	for ITT in $(seq 30)
	do
		export ITT
		for OMP_NUM_THREADS in $(seq 1 8)
		do
			export OMP_NUM_THREADS
			make -j test
		done
	done
}

export ENABLE_SPEC="1"
export RESULTS="$PWD/../results"
run

export ENABLE_SPEC=""
export RESULTS="$PWD/../nonspec_results"
run
