#!/bin/bash -ex

thousand=","
mythreads=$(seq 0 7)

#~ events="cpu/cycles,in_tx_cp=1/ cpu/L1-dcache-load-misses,in_tx_cp=1/ cpu/L1-dcache-store-misses,in_tx_cp=1/ cpu/cache-misses,in_tx_cp=1/ cpu/cycles,in_tx_cp=1/ cpu/instructions,in_tx_cp=1/ cpu/cpu-clock,in_tx_cp=1/ cpu/task-clock,in_tx_cp=1/ cpu/page-faults,in_tx_cp=1/ cpu/minor-faults,in_tx_cp=1/ seconds"
events="cycles L1-dcache-load-misses L1-dcache-store-misses cpu/cache-misses,in_tx_cp=1/ cpu/instructions,in_tx_cp=1/ cpu-clock task-clock page-faults minor-faults seconds"

sEvents="specSuccess specPossible specFail specAbort specBlocked specConflict"

function t() {
	printf 'Command\t'
	for e in $events
	do
		printf "$e\t"
	done
	
	for t in $mythreads
	do
		for s in $sEvents
		do
			printf "$s$t\t"
		done
	done
	
	echo -e 'Time\trapl0\trapl00\trapl01\tthreads\tENABLE_SPEC\tITT'
}

function i() {
	for v in "$@"
	do
		#~ printf \"
		grep -o ".*$v" out.log | sed "s!$v!!g" | sed 's! *!!g' | tr -d "$thousand" | tr -d '\n' | tr ',' '.'
		#~ printf \"'\t'
		printf '\t'
	done
	
	for thread in $mythreads
	do
		if grep "^$thread:" out.log > out.$thread.log ;
		then
			for s in $sEvents
			do
				if grep -o "$s* [0-9]*" out.$thread.log > out.$thread.grep.log ;
				then
					tr -d '[a-zA-Z ]' <  out.$thread.grep.log | tr '\n' '\t'
				else
					printf '\t'
				fi
			done
		else
			for s in $sEvents
			do
				printf '\t'
			done
		fi
	done
	grep -Eo 'Time Program *= *[0-9.,]*' out.log | sed 's!Time Program *= *!!' | tr -d '\n'
}

function o() {
	printf \""$1\"\t" 
	shift
	i "$@"
}
