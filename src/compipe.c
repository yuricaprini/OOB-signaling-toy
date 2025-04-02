#include "compipe.h"

int createPipe(int * pfd)
{
	if( pfd==NULL ) { errno=EINVAL; return -1; }
	return pipe(pfd);
}

int sendMessageOnPipe(int p, message_t* msg){
	
	char* sendbuffer;	  	/*buffer ausiliario per la serializzazione*/
	int   size_sendbuffer;  /*dimensioni del buffer ausiliario*/
	int   bytes;			/*numero di byte scritti*/
	int   dim_pipe;		  	/*dimensione della pipe*/
	
	/*controllo degli argomenti*/
	ec_neg  ( p,   errno = EINVAL; return -1; ) 
	ec_null ( msg, errno = EINVAL; return -1; )
	
	/*per garantire l'atomicità, la lunghezza del messaggio non deve superare la capacità massima della pipe */
	size_sendbuffer = sizeof(unsigned int) + ( (msg->length) * sizeof(char) );
	errno = 0;
	ec_neg	( dim_pipe = fpathconf(p,_PC_PIPE_BUF), if (errno!=0) 	return -1; )
	ec_true	( size_sendbuffer > dim_pipe, 			errno=EMSGSIZE; return -1; )

	/*allocazione del buffer ausiliario*/
	ec_null ( sendbuffer = malloc(size_sendbuffer), return -1; )
	
	/*serializzazione*/	
	memcpy( sendbuffer, &(msg->length), sizeof(unsigned int) );
	memcpy( sendbuffer+sizeof(unsigned int), msg->buffer, (msg->length)* sizeof(char));

	/*scrittura sulla pipe*/
	bytes = write( p, sendbuffer, size_sendbuffer );
	ec_neg ( bytes, free(sendbuffer); return -1; )
	
	free(sendbuffer);
	
	return bytes;
}

int receiveMessageOnPipe(int p, message_t *msg){
	
	int bytes; /*numero di byte letti*/

	/*lettura della lunghezza del messaggio logico*/
	bytes=read( p, (&(msg)->length), sizeof(unsigned int) );
	ec_true( bytes==0,	errno=ENOTCONN; return 0; )
	ec_neg ( bytes, 	return -1; )
	
	/*alloco il buffer della struttura msg in base alla dimensione letta precedentemente*/
	ec_null( msg->buffer= malloc( (msg->length) * sizeof(char) ), return -1; )

	/*lettura dei byte significativi*/
	bytes = read( p, msg->buffer , (msg->length) );
	ec_true( bytes==0,	return -1; )
	ec_neg ( bytes, 	return -1; )

	return bytes; 
}

int closePipe(int p){
	return close(p);
}
