#include <stdlib.h>
#include <omp.h>

void spin(volatile int t) {
	while(--t > 0) { }
}

int main(int argc, char** argv) {
	unsigned ITT = 1000000;
	int s = 100;
	if(argc > 1) {
		ITT = atoi(argv[1]);
	}
	if(argc > 2) {
		s = atoi(argv[2]);
	}
#pragma omp parallel
	{
		unsigned id = omp_get_thread_num();
		for(unsigned i = 0; i != ITT; i++) {
			if(((id + i) & 1) != 0)
				spin(s);
#pragma omp barrier
		}
	}
}
