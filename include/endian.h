#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>

int check_for_endianness();

uint64_t hton64( uint64_t host64 );

uint64_t ntoh64( uint64_t network64 );
