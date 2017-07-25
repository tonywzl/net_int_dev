/*
 * mem_consume.c
 *
 *  A simple program for consuming system memory.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char** argv)
{
	int i = 0;
	if ( argc < 2 ) {
		printf("Please enter a argument for specify how much mega bytes memeory want to eatting\n");
		return;
	}
	int max = atoi(argv[1]);
	while (i++ < max)
	{
		memset(malloc(1024*1024), 'w', 1024*1024);
	}

	while ( 1 ) {
		sleep(1000);
	}
}


