// This program prints the first 10 random numbers from rand(), drand48(), and random()

#include <stdio.h>
#include <stdlib.h>

int main( int argc, char** argv )
{
	int i;

	srand( 42 );
	srand48( 42 );
	srandom( 42 );

	for( i = 0; i < 10 ; i++ )
		printf( "%d:  srand() = %10d,  drand48() = %06.4f,  random() = %10ld\n", i, rand(), drand48(), random() );

	return( 0 );
}
