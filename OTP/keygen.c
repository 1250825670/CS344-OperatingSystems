/*
Author: Siara Leininger
Class: CS 344
Assignment: Program 4 - OTP
The program creates a key file of specified length
(26 capital letters, and the space character)
the error is printed to stderr
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//main function start C program
int main(int argc, char *argv[]){

	int i; //loop control variable
	int keylength; //key length
	char* STRING = " ABCDEFGHIJKLMNOPQRSTUVWXYZ"; //valid characters

	//initialize random number generator
	srand(time(NULL));

	if (argc != 2) { fprintf(stderr,"USAGE: %s <keylength>\n", argv[0]); exit(0); } // Check usage & args

	//validate keylength
	if (sscanf(argv[1], "%d", &keylength) != 1 || keylength <= 0)
	{
		fprintf(stderr,"Error: keylength must be positive integer number\n"); exit(0);
	}

	//print [keylength] random characters
	for (i = 0; i < keylength; i++)
	{
		printf("%c", STRING[rand() % 27]);
	}
	printf("\n"); //print a new line character

	return 0;
}
