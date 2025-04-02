#include "internals.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define ERR_NO_NUM -1
#define ERR_NO_MEM -2

uint64_t rand_uint64()
{
	uint64_t piece, result;
	int i;
	
	/*setto a zero le variabili per una composizione "pulita"*/
	memset(&piece, 0, sizeof(uint64_t));
	memset(&result, 0, sizeof(uint64_t));

	/*composizione del numero*/
	for (i = 0; i < sizeof(uint64_t); i++ )
	{
		piece= (uint64_t) rand() << 56 >> (8*i);
		result= result | piece;
	}

	return result;
}

void milliSleep(int ms) {

	struct timespec req;
	int sec;

	memset(&req, 0, sizeof( struct timespec ) );

	/*conversione in millisecondi*/
	sec = ms / 1000;
	ms = ms - sec * 1000;
	req.tv_sec = sec;
	req.tv_nsec = ms * 1000000;
	
	/*se la nanosleep è interrotta da un segnale rieffettuo la chiamata con il tempo rimanente*/
	while(( nanosleep(&req, &req) == -1 ) && ( errno==EINTR )) {}
	
	/*se la condizione è verificata la milliSleep ha avuto successo*/
	if (errno==EINTR)
	{
		errno=0;
	}
}

int randFromPool( int* size, int* pool ){
	int rand_index;
	int elem_from_pool;

	if (size==NULL || *size==0 )
	{
		errno=EINVAL;
		return -1;
	}
	
	if ( pool==NULL )
	{
		errno=EINVAL;
		return -1;
	}

	/*ottengo un elemento dalla pool in modo pseudocasuale*/
	rand_index = rand() % (*size);
	elem_from_pool = pool[ rand_index ];
		
	/*rimuovo l'elemento dalla pool scambiandolo con l'ultimo */
	pool[ rand_index ]= pool[(*size)-1];
	(*size)--;

	return elem_from_pool;	
	
}

/*
void printlist (arrives_list_t *l) {
  arrives_list_t *s;

  s=l;
  while(s!=NULL) {
	
    printf("sec:%" PRIuMAX " usec:%.06" PRIuMAX "---", (uintmax_t) (*s).tv.tv_sec,(uintmax_t)(*s).tv.tv_usec);
    s=(*s).next;
  }

  printf("\n");
}
*/

uint32_t euclide_MCD(uint32_t a, uint32_t b)
{
	uint32_t r;    

	if (b == 0) return a; 

    r = a % b;             
    while(r != 0)          
    {
        a = b;
        b = r;
        r = a % b;
    }
    return b;
}

void calculateIntervals (arrives_list_t *l, uint32_t* differences) {

  arrives_list_t *pre;
  arrives_list_t *succ;
  
  int i;
  uintmax_t pre_usec, succ_usec ,pre_sec , succ_sec;
	
  uint32_t diff_millisec;
	
  uintmax_t pre_union, succ_union;

  pre=l;
  succ=l->next;

  i=0;	
  while(succ!=NULL) {
	
	pre_sec   = (uintmax_t)  pre->tv.tv_sec;
	succ_sec  = (uintmax_t)  succ->tv.tv_sec;
	
	pre_usec  = (uintmax_t) pre->tv.tv_usec;
	succ_usec = (uintmax_t) succ->tv.tv_usec;
	
	pre_union= (pre_sec*1000000)+pre_usec;
	succ_union= (succ_sec*1000000)+succ_usec;
	
	diff_millisec=pre_union-succ_union;
	diff_millisec= diff_millisec/1000;

	#ifdef DEBUG  
	printf("diff_millisec:%" PRIu32 "\n",diff_millisec);
	#endif	
	
	differences[i]= diff_millisec;
	
	i++;
	pre  = pre->next;    
	succ = succ->next;
	
  }

}

