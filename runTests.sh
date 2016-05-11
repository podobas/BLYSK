#!/bin/bash
if [ "$#" -lt 1 ]
then
	echo "Usage:"
	echo "$0 PATH_TO_TESTS"
	exit 1
fi

function memdebug() {
	test "$#" -ge 1 || exit
	valgrind -v --fair-sched=yes --track-origins=yes --leak-check=full --show-leak-kinds="all" "$@" |& tee valgrind.log | less
}

function insdebug() {
	test "$#" -ge 1 || exit
	valgrind --fair-sched=yes --tool=cachegrind --cache-sim=yes "$@"
	cg_annotate --show='Dw' $(ls -t cachegrind.out.* | head -n 1) --auto=yes --threshold=2|less
	cg_annotate --show='Ir' $(ls -t cachegrind.out.* | head -n 1) --auto=yes |less
}

function perfdebug() {
	test "$#" -ge 1 || exit
	rm -f perf*
	#~ perf record  --call-graph dwarf -F 39 -e L1-dcache-load-misses -e L1-dcache-store-misses -e cache-misses -e cycles -e instructions -e cpu-clock -e task-clock -e page-faults -e minor-faults "$@"
	#~ perf record  --call-graph dwarf -F 39  -e task-clock "$@"
	#~ perf report
	#~ perf report -x --parent="malloc|free"
	#~ perf report -x --parent="pthread"
	perf stat -e L1-dcache-load-misses -e L1-dcache-store-misses -e cache-misses -e cycles -e instructions -e cpu-clock -e task-clock -e page-faults -e minor-faults "$@" 
}

export PATH="$PWD/shell:$PATH"

function perftot() {
	test "$#" -ge 1 || exit
	mkdir -p $RESULTS
	c.sh "$@" >> $RESULTS/$1.$ENABLE_SPEC.tsv
}


set -xe
. env.sh

# Default argument sizes taken from Adaptive Granularity Control in Task Parallel Programs Using Multiversioning
#~ pushd ../bots/omp-tasks/sparselu/sparselu_single
pushd ../bots/
#~ make clean
#~ make  LIBS="$LIBS"
ionice -c 3 nice -n 19 make -ki LIBS="$LIBS"
#~ ionice -c 3 nice -n 19 make LIBS="$LIBS"
cd bin

exit 1
#~ perftot ./fib.gcc.serial

#~ perftot ./alignment.gcc.single-omp-tasks -f ../inputs/alignment/prot.100.aa
#~ perftot ./fft.gcc.omp-tasks -n 536870912 # Memory leak?
#~ perftot ./fib.gcc.omp-tasks-manual -n 48
#~ perftot ./floorplan.gcc.omp-tasks-manual -f ../inputs/floorplan/input.20 
#~ perftot ./health.gcc.omp-tasks-manual -f ../inputs/health/large.input
#~ perftot ./nqueens.gcc.omp-tasks-manual -n 13
#~ perftot ./sort.gcc.omp-tasks -n 134217728
#~ perftot ./sparselu.gcc.single-omp-tasks
#~ perftot ./strassen.gcc.omp-tasks-manual -n 4096
perftot ./uts.gcc.omp-tasks -f ../inputs/uts/tiny.input

#~ perftot ./alignment.gcc.single-omp-tasks -f ../inputs/alignment/prot.100.aa -c
#~ ./fft.gcc.omp-tasks -c # -n 536870912
#~ ./fib.gcc.omp-tasks-manual -c # -n 48
#~ ./floorplan.gcc.omp-tasks-manual -c -f ../inputs/floorplan/input.20 
#~ ./health.gcc.omp-tasks-manual -f ../inputs/health/large.input -c
#~ ./nqueens.gcc.omp-tasks-manual -c -n 13
#~ ./sort.gcc.omp-tasks -c #-n 134217728
#~ ./sparselu.gcc.single-omp-tasks -c
#~ ./strassen.gcc.omp-tasks-manual -c # -n 4096
#~ ./uts.gcc.omp-tasks -c -f ../inputs/uts/tiny.input

popd

dir="$1"

cd "$dir"
tests="$(ls */ -d | sort -r)"
for t in $tests
do
	pushd "$t"
	ln -sf "$LD_LIBRARY_PATH/libblysk.a"
	#~ make clean
	#~ make -j 4 all
	if echo $t | grep '^UC_' ; then
		cd apps
		prog='bash ../test.sh'
		#~ prog='echo hi'
	else
		prog="$(find . -maxdepth 1 -perm -111 -type f)"
	fi
	
	#~ $prog || (gdb $prog;true)
	popd
done

#~ pushd MatMul
#~ perfdebug
#~ ./matmul
#~ popd

#~ pushd Hurdler
#~ perfdebug ./Hurdler 10000000
#~ popd

#~ pushd SimpleTest
#~ perfdebug ./SimpleTest 10000000
#~ OMP_NUM_THREADS=1 insdebug ./SimpleTest 10000
#~ OMP_NUM_THREADS=1 memdebug ./SimpleTest 10000
#~ popd

#~ pushd UC_RadarDet/apps/
#~ for i in $(seq 40)
#~ do
	#~ true
	#~ ./radar_sigproc input.csv output.csv
#~ done
#~ popd

