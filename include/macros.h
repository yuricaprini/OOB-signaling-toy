#ifndef MACROS_H
#define MACROS_H

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#define FALSE 0
#define TRUE 1

#define ec_false(e,c) \
	if ( (e) == FALSE ) { c }

#define ec_true(e,c) \
	if ( (e) == TRUE ) { c }

#define ec_neg1(e,m) \
	if ( (e) == -1 ) { perror(m); exit(EXIT_FAILURE); }

#define ec_neg(e,c) \
	if ( (e) < 0 ) { c }

#define ec_null(e,c)\
	if ( (e) == NULL ) { c }
 
#endif


