#include "endian.h"


int check_for_endianness()
{
  unsigned int x = 1;
  char *c = (char*) &x;
  return (int)*c;
}

uint64_t hton64( uint64_t host64 )
{
	uint32_t low, high;

	if ( check_for_endianness() ){
		
		/* la macchina host è Little-endian (I byte meno significativi sono memorizzati in indirizzi minori).
		   E' necessaria la conversione Little endian-> Big endian di host64 */
		low=((uint32_t *)&host64)[0];
		high= ((uint32_t *)&host64)[1];

		((uint32_t *)&host64)[0]=htonl(high);
		((uint32_t *)&host64)[1]=htonl(low);

	}
	return host64;
} 

uint64_t ntoh64( uint64_t network64 )
{
	uint32_t low, high;

	if ( check_for_endianness() ){

		/* la macchina host è Little-endian (I byte meno significativi sono memorizzati in indirizzi minori).
		   Necessaria la conversione Big endian a Little endian di network64 */
		   
		high=((uint32_t *)&network64)[0];
		low= ((uint32_t *)&network64)[1];
	
		((uint32_t *)&network64)[0]=ntohl(low);
		((uint32_t *)&network64)[1]=ntohl(high);

	}
	return network64;	
} 

