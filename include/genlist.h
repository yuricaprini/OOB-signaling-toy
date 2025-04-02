#ifndef _GENLIST_H
#define _GENLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Struttura che rappresenta 
 * il nodo di una lista generica
 */
typedef struct gennode {
	void * elem ; 			/**< puntatore generico all'elemento contenuto nel nodo */
	struct gennode * next ; /**< puntatore al prossimo elemento della lista*/

}gennode_t;

/** Tipo lista generica */
typedef gennode_t * genlist_t;

/** Funzione di inserimento in testa di una lista generica.
 *	
 *	Dato un puntatore alla testa della lista e il puntatore
 * 	ad un nuovo elemento, effettua un inserimento in testa e
 *  restituisce il puntatore alla nuova lista.
 *  
 *	/param  head 		puntatore alla testa della lista
 *  /param  elem 		puntatore al nuovo elemento
 *
 *	/retval new_head 	puntatore alla nuova lista
 *	/retval NULL 		in caso di errore
*/
genlist_t insertList( genlist_t head, void* elem );

/** Procedura di deallocazione di una lista generica.
 *	
 *	Dato un puntatore alla testa della lista dealloca
 *  ogni nodo della lista, strutture interne comprese.
 *
 *	/param  head 		puntatore alla testa della lista
 *
 *	/retval void
 */
void freeList( genlist_t head );

/** Procedura di stampa di una lista generica.
 *	
 *	Prende come argomenti il puntatore alla testa di una lista
 *	e il puntatore funzione di stampa funprint.
 * 	Quest'ultima indica il tipo di cast e il formato di stampa
 *  da applicare al campo elem di ogni nodo della lista.
 *  Stampa l'intera lista.
 *
 *	/param  head 		puntatore alla testa della lista
 *	/param	funprint	puntatore alla funzione di stampa
 *						del campo elem di ogni nodo della
 *						lista.
 *
 *	/retval void
 */
void printList ( genlist_t head, void (*funprint) ( void * elem ) );


#endif
