#ifndef _RAND_U64_H
#define _RAND_U64_H

/* Secondo la documentazione di stdlib.h la macro RAND_MAX è garantito essere almeno 32767 e varia in base al tipo di architettura*/
#include <inttypes.h>
#include <sys/time.h>
#define PRI_HEX_SUFFIX "%#.16"

typedef struct arrives_list
{
	struct timeval tv;
	struct arrives_list *next;
} arrives_list_t;

/*
void printlist (arrives_list_t *l);
*/

/** Crea un intero positivo pseudocasuale a 64 bit.
 *
 *  La funzione utilizza chiamate multiple alla rand() per comporre l'intero a 64 bit.
 *	E' necessario quindi, che la funzione srand() sia chiamata preventivamente all'esterno della funzione.
 *	\param  void
 *
 *  \retval result	l'unsigned int a 64 bit	appena creato.
 */
uint64_t rand_uint64();

/** Sospende l'esecuzione del thread chiamante per ms millisecondi.
 *
 *	La funzione è in sostanza un wrapper della funzione nanosleep(). Facendo uso delle caratteristiche di quest'ultima,
 *	milliSleep() non si interrompe nel caso in cui il thread chiamante riceva un segnale.
 *	\param void
 *
 *	\return void	setta opportunamente errno in caso di errore.
 */
void milliSleep(int ms);

int randFromPool(int *size, int *pool);

uint32_t euclide_MCD(uint32_t a, uint32_t b);

void calculateIntervals(arrives_list_t *l, uint32_t *differences);

#endif
