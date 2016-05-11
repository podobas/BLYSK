export LD_LIBRARY_PATH="$PWD"
export LIBS="-lm -L$PWD -lblysk -ldl -pthread"
export BLYSK_PATH="$PWD/libblysk.a"
export OMP_TASK_SCHEDULER=worksteal_stack
