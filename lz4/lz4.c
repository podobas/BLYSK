/*
 * Not integrated yet
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define DEFAULT_LITERAL_SIZE 15

#define HMAP_SIZE 16000000


static int num_acc = 0, num_hits = 0;

struct __hmap_entry {
  unsigned char *ptr;
  unsigned char nb;
  short adr;
  unsigned char active;
};

struct __hmap_entry H_Map[HMAP_SIZE];

void hash_clear ( void )
{
  int i;
  for (i=0;i<HMAP_SIZE;i++)
    H_Map[i].active = 0;
}

void hash_add (unsigned char *seq, short _adr, unsigned char nb)
{
  unsigned int HMAP_f = 0;
  int i;
  for (i=0;i<nb;i++)
    HMAP_f += (seq[i] << (i*8));

  HMAP_f = HMAP_f % HMAP_SIZE;
  
  H_Map[HMAP_f].active = 1;
  H_Map[HMAP_f].nb = nb;
  H_Map[HMAP_f].adr = _adr;
  H_Map[HMAP_f].ptr = seq;  
}

short hash_find (unsigned char *seq, unsigned char nb)
{
  unsigned int HMAP_f = 0;
  int i;
  for (i=0;i<nb;i++)
    HMAP_f += (seq[i] << (i*8));

  HMAP_f = HMAP_f % HMAP_SIZE;

  if (H_Map[HMAP_f].active == 0) return 0;

  /* Vectorizable */
  for (i=0;i<nb;i++)
    if (H_Map[HMAP_f].ptr[i] != seq[i])
      return 0;
  return H_Map[HMAP_f].adr;
}


short find_match   (char *seq , int *Lits, int *MLits, int cur_pos)
{
  num_acc ++;
  *MLits = 0;
  *Lits = DEFAULT_LITERAL_SIZE;
  short t1;
  int i;
  int j;
  for (i=10;i>5;i--)
  for (j=0;j<5;j++)
    {
      t1 = hash_find (&seq[j], i);
      if (t1) goto good;
    }
 good:
  if (t1 == 0) 
    return 0;

  *MLits = i;
  *Lits = 4;
  num_hits++;
  return cur_pos - t1;
}



short find_match_x (unsigned char *outstream, int out_size, 
		    unsigned char *instream, unsigned char *instreamend,
		    unsigned char *matchedLits, int *Lits)
{
  short offset = 0;
  int outstream_iter;
  *Lits = DEFAULT_LITERAL_SIZE;
  *matchedLits = 0;
  
  outstream_iter = out_size - 65535;
  if (outstream_iter < 0) outstream_iter = 0;

  for (; outstream_iter < out_size; outstream_iter++)
    {
      int instream_iter;
      for (instream_iter = 0; instream_iter < DEFAULT_LITERAL_SIZE && &instream[instream_iter] != instreamend;   instream_iter++)
	{
	  unsigned char n_matches = 0;
	  int outstream_iter_1 = outstream_iter;
	  int instream_iter_1 = instream_iter;
	  while (1) {
	  if (outstream[outstream_iter_1] != instream[instream_iter_1])
	    break;
	    n_matches++;
	    outstream_iter_1++;
	    instream_iter_1++;	   
	    if (outstream_iter_1 == out_size) 
	      break;
	    if (instream_iter_1 >= (DEFAULT_LITERAL_SIZE*2)-1 ) 
	      break;
	  }
	  if (n_matches > *matchedLits) {
	    *matchedLits = n_matches;
	    *Lits = instream_iter;
	    offset = out_size - outstream_iter;
	  }
	    
	}     
    }
  return offset;
}

int blysk__lz4_encrypt ( unsigned char *in, int _size, unsigned char *out )
{

  int lit;
  int out_ptr = 0;

  for (lit = 0; lit < _size; )
    {
      unsigned char token = 0x0;
      unsigned lit_size = 0;
      short offset = 0; 
      unsigned char matched = 0;

      /* Attempt find a match */
      //      offset = find_match_x (out, out_ptr, &in[lit], &in[_size], &matched, &lit_size);
      offset = find_match(&in[lit], &lit_size, &matched, lit);
      /* Generate token */
      token |= lit_size << 4;
      token |= matched;
      out[out_ptr++] = token;

      /* Copy into stream */
      if (lit_size)
	{
	  memcpy(&out[out_ptr], &in[lit], lit_size);
	  out_ptr+=lit_size;
	}     
      if (matched)
	{
	  memcpy (&out[out_ptr], &offset, sizeof(short));
	  out_ptr+=sizeof(short);
	}
      int i;
      if (!matched)
      for (i=5;i<11;i++)
      hash_add(&out[out_ptr-i], i, out_ptr);
      lit += (matched + lit_size);
    }

  return out_ptr;
}

int blysk__lz4_decrypt (unsigned char *in, int _size, unsigned char *out)
{
  int lit;
  int out_ptr = 0;

  for (lit = 0; lit < _size; )
    {
      unsigned char token = 0x0;
      short offset;

      token = in[lit];
      lit++;

      int lit_size = token >> 4;
      int matched = token & 0xf;
      int off_pos = lit;

      if (lit_size)
	{
	  memcpy (&out[out_ptr], &in[lit], lit_size);	  
	  lit += lit_size;
	  out_ptr += lit_size;
	}
      if (matched)
	{	  
	  memcpy (&offset , &in[lit] , sizeof(short));	  
	  memcpy( &out[out_ptr], &in[off_pos - offset - 1], matched);
	  lit += 2;
	  out_ptr += matched;
	}
    }
  return out_ptr;  
}

int main ( int argc, char *argv[])
{
  hash_clear();

  FILE *fp = fopen(argv[1],"r");
  fseek (fp , 0 , SEEK_END);
  unsigned int file_size = ftell (fp);
  rewind (fp);
  
  unsigned char *buf_in = (unsigned char *) malloc(file_size);
  unsigned char *buf_crypt = (unsigned char *) malloc(file_size*2);
  unsigned char *buf_out = (unsigned char *) malloc(file_size);
  
  fread(buf_in,1,file_size,fp);

  fclose(fp);
  

  fprintf(stderr,"Uncompressed size: %d\n",file_size);

  double _t1 = omp_get_wtime();
  int size = blysk__lz4_encrypt ( buf_in, file_size , buf_crypt);
  double _t2 = omp_get_wtime();

  fprintf(stderr,"Compressed size: %d\n",size);

  size = blysk__lz4_decrypt(buf_crypt, size, buf_out);


  fprintf(stderr,"De-compressed size: %d\n",size);


  fprintf(stderr,"\nHits: %d / %d\n",num_hits, num_acc);
  fprintf(stderr,"MB/s: %f\n",(double) file_size / (_t2-_t1) / 1024.0 / 1024.0);
  return 0;
}
