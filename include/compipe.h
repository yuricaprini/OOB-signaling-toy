#include "macros.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/** <H3>Messaggio</H3>
 * La struttura \c message_t rappresenta un messaggio 
 * - \c length rappresenta la lunghezza in byte del campo buffer
 * - \c buffer e' il puntatore al messaggio (puo' essere NULL se length == 0)
 *
 * <HR>
 */
typedef struct {
    unsigned int length; /** lunghezza in byte, escluso il carattere terminatore */
    char* buffer;        /** buffer messaggio */
} message_t; 

/** Crea la pipe.
 * 
 * 	Prende come argomento un array di due posizioni pdf e lo riempie con i file descriptor
 * 	della pipe.
 *	La funzione è solo un wrapper di pipe() ed è implementata per fini di completezza della libreria
 *
 *	\param  	pfd 	array dei file descriptor
 *
 *	\retval		0 		se l'operazione ha avuto successo
 *	\retval	   -1		in caso di errore (setta errno)
 */
int createPipe(int * pfd);

/** Scrive un messaggio sulla pipe. La funzione spacchetta la struttura msg e serializza il contenuto dei suoi campi in un unico array, che 	viene scritto sulla pipe con un'unica write

 *   \param  	p 		 file descriptor (in scrittura) della pipe
 *   \param 	msg 	 indirizzo della struttura che contiene il messaggio da scrivere 
 *   
 *   \retval  	bytes    il numero di caratteri inviati (se scrittura OK)
 *   \retval 	-1   	 in caso di errore (setta errno )
 *				  		 errno = EMSGSIZE se il messaggio è più lungo della capacità della pipe
 */
int sendMessageOnPipe(int p, message_t* msg);

/** Legge un messaggio dalla pipe. Il messaggio è adeguatamente trasferito nella struttura msg (Il cui buffer è allocato all'interno della 		funzione stessa).

 *  \param  	sc  	file descriptor (in lettura) della pipe
 *  \param 		msg  	indirizzo della struttura che conterra' il messaggio letto 
 *              		(deve essere allocata all'esterno della funzione)
 *
 *  \retval 	bytes 	numero dei byte letti ( i facenti parte del contenuto del messaggio )
 *  \retval  	-1   	in caso di errore (setta errno)
 *              	   	errno = ENOTCONN se il peer ha chiuso la connessione 
 *              	    (non ci sono piu' scrittori sulla pipe)
 */
int receiveMessageOnPipe(int p, message_t * msg);

/** Chiude la pipe in lettura o scrittura
 *
 *	\param		p		file descriptor ( in lettura o scrittura) della pipe
 *	
 *	\retval		0		se l'operazione ha successo
 *	\retval	   -1		in caso di errore (setta errno)
 */
int closePipe(int p);
