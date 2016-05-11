

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <clapack.h>
#include <cblas.h>
#include <omp.h>

#define DIM 16
#define NB 256

//#define DIM 96
//#define NB 128


extern void spotrf_(char *, int *, float *, int *, int *);


/*#pragma css task input(NB) inout(A[NB][NB]) highpriority */
void smpSs_spotrf_tile(float *A)
{

unsigned char LO='L';
int INFO;
int nn=NB;
   spotrf_(&LO,
          &nn,
          A,&nn,
          &INFO);
}

/*#pragma css task input(A[NB][NB], B[NB][NB], NB) inout(C[NB][NB])*/
void smpSs_sgemm_tile(float  *A, float *B, float *C)
{
unsigned char TR='T', NT='N';
float DONE=1.0, DMONE=-1.0;
    cblas_sgemm(
        CblasColMajor,
        CblasNoTrans, CblasTrans,
        NB, NB, NB,
        -1.0, A, NB,
              B, NB,
         1.0, C, NB);

}

/*#pragma css task input(T[NB][NB], NB) inout(B[NB][NB])*/
void smpSs_strsm_tile(float *T, float *B)
{
unsigned char LO='L', TR='T', NU='N', RI='R';
float DONE=1.0;

    cblas_strsm(
        CblasColMajor,
        CblasRight, CblasLower, CblasTrans, CblasNonUnit,
        NB, NB,
        1.0, T, NB,
             B, NB);

}

/*#pragma css task input(A[NB][NB], NB) inout(C[NB][NB])*/
void smpSs_ssyrk_tile( float *A, float *C)
{
unsigned char LO='L', NT='N';
float DONE=1.0, DMONE=-1.0;

    cblas_ssyrk(
        CblasColMajor,
        CblasLower,CblasNoTrans,
        NB, NB,
        -1.0, A, NB,
         1.0, C, NB);

}


void compute(struct timeval *start, struct timeval *stop, float *A[DIM][DIM])
{
#pragma omp parallel num_threads(48) 
#pragma omp single
{
 gettimeofday(start,NULL);
  double t1 = omp_get_wtime();

  long j,k,i;
  for (j = 0; j < DIM; j++)
  {
    for (k= 0; k< j; k++)
    {
      for (i = j+1; i < DIM; i++) 
      {
      #pragma omp task shared(A) firstprivate(i,k,j)  depend(in : A[i][k], A[j][k]) depend(inout:A[i][j]) 
        smpSs_sgemm_tile(  A[i][k], A[j][k], A[i][j]);
      }
    }
    for (i = 0; i < j; i++)
    {
    #pragma omp task shared(A) firstprivate(i,j) depend (in : A[j][i]) depend(inout : A[j][j])
      smpSs_ssyrk_tile( A[j][i], A[j][j]);
    }

   #pragma omp task shared(A) firstprivate(j) depend(inout : A[j][j])
    smpSs_spotrf_tile( A[j][j]);

    for (i = j+1; i < DIM; i++)
    {
  #pragma omp task shared(A) firstprivate(i,j) depend(in : A[j][j]) depend (inout : A[i][j])
      smpSs_strsm_tile( A[j][j], A[i][j]);
    }
   
  }
#pragma omp taskwait
	
  double t2 = omp_get_wtime();
  fprintf(stderr,"Time: %f\n",t2-t1);
}
 gettimeofday(stop,NULL);
 exit(0);
}


static void init(int argc, char **argv,  long *N_p);

float **A;
float * Alin; 


long N;

int
main(int argc, char *argv[])
{
 
unsigned char LO='L';
int  INFO;
 
  struct timeval start;
  struct timeval stop;
  unsigned long elapsed;

  init(argc, argv, &N);
  fprintf(stderr,"Computing cholesky...\n");

  compute(&start, &stop, (void *)A);

  int nn=N;

  elapsed = 1000000 * (stop.tv_sec - start.tv_sec);
  elapsed += stop.tv_usec - start.tv_usec;

  printf ("%lu;\t", elapsed);
  printf("%d\n", (int)((0.33*N*N*N+0.5*N*N+0.17*N)/elapsed));

  printf("par_sec_time_us:%lu\n",elapsed);
  return 0;
}


static void convert_to_blocks(long N, float *Alin, float *A[DIM][DIM])
{
  long i,j;
  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
    {
      A[j/NB][i/NB][(i%NB)*NB+j%NB] = Alin[i*N+j];
    }
  }

}



void fill_random(float *Alin, int NN)
{
  int i;
  for (i = 0; i < NN; i++)
  {
    Alin[i]=((float)rand())/((float)RAND_MAX);
  }
}


static void init(int argc, char **argv,  long *N_p)
{
  long ISEED[4] = {0,0,0,1};
  long IONE=1;


  
  
  long N = NB*DIM;
  long NN = N * N;

  *N_p = N;

   Alin = (float *) malloc(NN * sizeof(float));

  fill_random(Alin,NN);
  long i;
  for(i=0; i<N; i++)
  {
    Alin[i*N + i] += N;
  }
  
  A = (float **) malloc(DIM*DIM*sizeof(float *));
  for ( i = 0; i < DIM*DIM; i++)
    {
//     #pragma omp memory
     A[i] = (float *) malloc(NB*NB*sizeof(float));
      int z;
      for (z=0;z<DIM*DIM;z++)
	   { float *zz = (float *) A[i];
	     zz[z] = Alin[(DIM*DIM)*i + z]; }
    }

  convert_to_blocks(N, Alin, (void *)A);

}
