CC=gcc

FLAGS  = -std=c99 -O3 -fopenmp -Wall -Wno-unused 
#LIBS   = -Wl,-lcblas,-latlas,-llapack,-lptcblas,-lptf77blas,-lgfortran,-lm,-lpapi

#LIBS   = -L/home/podobas/.libs/Atlas/lib/ -lgfortran -Wl,-lcblas,-latlas,-llapack,-lptcblas,-lf77blas,-lm

LIBS   = -Wl,-lgfortran,-llapack,-lptcblas,-lptf77blas,-lm,-lcblas,-latlas

TARGETS=cholesky



all: $(TARGETS)

cholesky: cholesky.c
	$(CC) $(FLAGS) -o $@ $< $(LIBS)

clean:
	$(RM) *.o *~ $(TARGETS)

