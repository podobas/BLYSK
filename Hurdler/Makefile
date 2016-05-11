CFLAGS ?= -O3
CFLAGS += -std=c99 -I../include
OMP_LIB ?= ../libblysk.a
# Alternatively: OMP_LIB := -fopenmp

all:
	gcc -c $(CFLAGS) -fopenmp main.c
	gcc $(CFLAGS) -o Hurdler main.o $(OMP_LIB)  -lpthread -ldl 

clean:
	rm -rf *.o valgrind.log cachegrind* perf* Hurdler

