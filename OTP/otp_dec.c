/*
Author: Siara Leininger
Class: CS 344
Assignment: Program 4 - OTP
File otp_enc.c
The program works are TCP Socket client
that sends the ciphertext, key to otp_dec_d to decrypt ciphertext
(encrypt using modulo 27 where 27 characters are the 26 capital letters, and the space character)
The output is written to the standard output
References: http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#setsockoptman
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h> 

#define BUFF_SIZE 256 //buffer size
#define LENGTH_SIZE 10 //10 characters notifying about length of data
#define HOST "localhost"
#define PROGRAM "otp_dec"

void error(const char *msg) { perror(msg); exit(2); } // Error function used for reporting issues, prints it to stderr 

/*
read [LENGTH_SIZE] charaters as length to receive
*/
int getLength(char *buffer){
	int length = 0; //length of data
	int i; //loop control variable    

	for (i = 0; i < LENGTH_SIZE; i++)
	{
		length = length * 10 + (buffer[i] - '0');
	}
	return length;
}

/*
set [LENGTH_SIZE] charaters as length to send
*/
void setLength(int length, char *buffer){
	char format[BUFF_SIZE];
	sprintf(format, "%%0%dd", LENGTH_SIZE);
	sprintf(buffer, format, length);	
}

/*
read into buffer from socket
precondition: the number of character received is less than BUFF_SIZE
return number of characters received
*/
int receiveData(char data[], int establishedConnectionFD){

	int totalCharReceived; //total number of characters received
	int charsRead;//chars read
	int length;//length of data
	char buffer[BUFF_SIZE];
	char bufferLength[BUFF_SIZE];

	//receive the length	
	totalCharReceived = 0;
	memset(bufferLength, '\0', BUFF_SIZE);

	//call recv until it gets [LENGTH_SIZE] characters
	while (totalCharReceived < LENGTH_SIZE)
	{
		memset(buffer, '\0', BUFF_SIZE);
		//receive data into buffer at [totalCharReceived] position, the number of characters received is less than or equal LENGTH_SIZE - totalCharReceived
		charsRead = recv(establishedConnectionFD, buffer, LENGTH_SIZE - totalCharReceived, 0); // Read the client's message from the socket
		if (charsRead < 0) error("ERROR reading from socket");

		if (charsRead == 0){
			break;
		}

		totalCharReceived += charsRead;
		buffer[charsRead] = '\0'; //append with '\0'
		strcat(bufferLength, buffer); //appen to bufferLength

		//printf("Debug: receive charsRead = %d totalCharReceived = %d\n", charsRead, totalCharReceived);
	}

	//check if it receives enough number of characters
	if (totalCharReceived != LENGTH_SIZE){
		error("Error: reading length from socket");
	}

	length = getLength(bufferLength); //convert to intege as data length

	//printf("Debug: receive length = %d\n", length);

	//length is zero
	if (length == 0)
	{
		return LENGTH_SIZE;
	}
	
	//receive data
	memset(data, '\0', BUFF_SIZE);
	totalCharReceived = 0;
	//call recv until it gets [totalCharReceived] characters
	while (totalCharReceived < length)
	{
		memset(buffer, '\0', BUFF_SIZE);
		//receive data into buffer at [totalCharReceived] position, the number of characters received is less than or equal length - totalCharReceived
		charsRead = recv(establishedConnectionFD, buffer, length - totalCharReceived, 0); // Read the client's message from the socket
		if (charsRead < 0) error("ERROR reading from socket");

		if (charsRead == 0){
			break;
		}

		totalCharReceived += charsRead;

		buffer[charsRead] = '\0'; //append with '\0'
		strcat(data, buffer); //appen to bufferLength
	}

	if (totalCharReceived != length){
		error("Error: reading data from socket");
	}

	data[length] = '\0'; //append with '\0'

	return length;
}

/*
send data in buffer to socket
precondition: the number of character sent is less than BUFF_SIZE, buffer is not empty
return number of characters sent
*/
int sendData(char *buffer, int establishedConnectionFD){
	int totalCharSent; //total number of characters sent
	int charsWrote;//chars wrote
	int length = strlen(buffer);//length of data
	char lengthBuffer[LENGTH_SIZE];
	char* lengthBufferPointer = lengthBuffer; //pointer to lengthBuffer
	char* bufferPointer = buffer;//pointer to buffer

	//create length buffer
	setLength(length, lengthBuffer);

	//send the length
	totalCharSent = 0;

	//call send until it sends [LENGTH_SIZE] characters
	while (totalCharSent < LENGTH_SIZE)
	{
		//receive data into buffer at [totalCharReceived] position, the number of characters received is less than or equal LENGTH_SIZE - totalCharReceived
		charsWrote = send(establishedConnectionFD, lengthBufferPointer, LENGTH_SIZE - totalCharSent, 0); 
		if (charsWrote < 0) error("ERROR writing to socket");

		if (charsWrote == 0){
			break;
		}

		totalCharSent += charsWrote;
		lengthBufferPointer += charsWrote;

		//printf("Debug: sent totalCharSent = %d\n", totalCharSent);
	}

	//printf("Debug: sent length = %d\n", totalCharSent);

	if (length == 0)
	{
		return LENGTH_SIZE;//success
	}

	//send data
	totalCharSent = 0;
	while (totalCharSent < length)
	{
		//receive data into buffer at [totalCharReceived] position, the number of characters received is less than or equal LENGTH_SIZE - totalCharReceived
		charsWrote = send(establishedConnectionFD, bufferPointer, length - totalCharSent, 0); 
		if (charsWrote < 0) error("Error: writing to socket");
		totalCharSent += charsWrote;
		bufferPointer += charsWrote;

		//printf("Debug: sent totalCharSent = %d\n", totalCharSent);
	}
	
	return length;
}

//main function start C program
int main(int argc, char *argv[]){
	
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	int input_ciphertext_fd, input_key_fd;    /* Input file descriptors */
	char program[BUFF_SIZE]; //program name
	char plaintext[BUFF_SIZE]; //plain text buffer
	char key[BUFF_SIZE]; //key buffer
	char ciphertext[BUFF_SIZE]; //cipher text
    int ret_ciphertext_in, ret_key_in, ret_out, ret_in; //read file return values
	char* plaintextAll = NULL; //all cipher text
	int i; //loop control variable

	if (argc != 4) { fprintf(stderr,"USAGE: %s <plaintext> <key> port\n", argv[0]); exit(2); } // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname(HOST); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("Error: opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){ // Connect socket to address
		fprintf (stderr, "Error: could not contact otp_dec_d on port %d\n", portNumber);
		exit(2);
	}

	//send otp_dec string
	strcpy(program, PROGRAM);
	ret_out = sendData(program, socketFD);

	if (ret_out != strlen(PROGRAM))
	{
		perror ("Error: send program name");
		exit(2);
	}

	ret_in = receiveData(program, socketFD);
	if (strcmp(program, PROGRAM) != 0)
	{
		perror ("Error: sent to invalid server\n");
		exit(2);
	}

	//open plain text file
    input_ciphertext_fd = open (argv [1], O_RDONLY);
    if (input_ciphertext_fd == -1) {
		perror ("Error: opening ciphertext file");
        exit(2);
    }

	//open key file
	input_key_fd = open (argv [2], O_RDONLY);
    if (input_key_fd == -1) {
		perror ("Error: opening key file");
		//close file
		close (input_ciphertext_fd);
        exit(2);
    }

	//read plain text and key
	memset(ciphertext, '\0', BUFF_SIZE); // Clear out the buffer array
	memset(key, '\0', BUFF_SIZE); // Clear out the buffer array

	while((ret_ciphertext_in = read (input_ciphertext_fd, &ciphertext, BUFF_SIZE - 1)) > 0){

		//ignore new line character
		if (ciphertext[ret_ciphertext_in - 1] == '\n')
		{
			ciphertext[ret_ciphertext_in - 1] = '\0';
			ret_ciphertext_in--;			
		}else{
			//append with '\0'
			ciphertext[ret_ciphertext_in] = '\0';
		}

		ret_key_in = read (input_key_fd, &key, ret_ciphertext_in);//read key

		//printf("Debug: ret_ciphertext_in = %d ret_key_in = %d\n", ret_ciphertext_in, ret_key_in);

		if (ret_ciphertext_in != ret_key_in)
		{
			fprintf (stderr, "Error: key '%s' is too short\n", argv [2]);
			//close files
			close (input_ciphertext_fd);
			close (input_key_fd);
			exit(2);
		}

		//check valid characters
		for (i = 0; i < ret_ciphertext_in; i++)
		{
			//validate, character must be space or A-Z
			if (ciphertext[i] != ' ' && !(ciphertext[i] >= 'A' && ciphertext[i] <= 'Z'))
			{
				//printf("Debug: ciphertext[i] = %u %c\n", ciphertext[i], ciphertext[i]);
				fprintf (stderr, "%s error: input contains bad characters\n", argv[0]);
				//close files
				close (input_ciphertext_fd);
				close (input_key_fd);
				exit(2);
			}

			//validate, character must be space or A-Z
			if (key[i] != ' ' && !(key[i] >= 'A' && key[i] <= 'Z'))
			{
				//printf("Debug: key[i] = %u %c\n", key[i], key[i]);
				fprintf (stderr, "%s error: input contains bad characters\n", argv[0]);
				//close files
				close (input_ciphertext_fd);
				close (input_key_fd);
				exit(2);
			}
		}


		//printf("Debug: ciphertext: %s length = %d\n", ciphertext, ret_ciphertext_in);
		//printf("Debug: key: %s length = %d\n", key, ret_key_in);

		//send ciphertext to server to encrypt
		ret_out = sendData(ciphertext, socketFD);

		if (ret_out != ret_ciphertext_in)
		{
			perror ("Error: send cipher text");
			//close files
			close (input_ciphertext_fd);
			close (input_key_fd);
			exit(2);
		}
		
		//printf("Debug: sent ciphertext: %s length = %d\n", ciphertext, ret_out);

		//send key to server to encrypt
		ret_out = sendData(key, socketFD);

		if (ret_out != ret_key_in)
		{
			perror ("Error: send key");
			//close files
			close (input_ciphertext_fd);
			close (input_key_fd);
			exit(2);
		}

		//printf("Debug: sent key: %s length = %d\n", key, ret_out);

		//printf("Debug: receive data...\n");
		//receive cipher text
		ret_in = receiveData(plaintext, socketFD);
		if (ret_out != ret_in)
		{
			perror ("Error: receive plaintext");
			//close files
			close (input_ciphertext_fd);
			close (input_key_fd);
			exit(2);
		}

		//append with '\0'
		plaintext[ret_in] = '\0';

		//printf("Debug: got plaintext: %s length = %d\n", plaintext, ret_in);

		//print to standard output
		//printf("%s", ciphertext);
		if (plaintextAll == NULL)
		{
			plaintextAll = (char*)malloc((ret_in + 1) * sizeof(char));
			strcpy(plaintextAll, plaintext);
		}else{
			plaintextAll = (char*)realloc(plaintextAll, (strlen(plaintextAll) + ret_in + 1) * sizeof(char));
			strcat(plaintextAll, plaintext);
		}		

		memset(ciphertext, '\0', BUFF_SIZE); // Clear out the buffer array
		memset(key, '\0', BUFF_SIZE); // Clear out the buffer array
	}//end while

	//print to standard output
	printf("%s", plaintextAll);

	//print new line standard output
	printf("\n");

	//send length = 0 to finish the transaction
	memset(ciphertext, '\0', BUFF_SIZE); // Clear out the buffer array
	//send length = 0 to server to encrypt
	ret_out = sendData(ciphertext, socketFD);

	if (ret_out != LENGTH_SIZE)
	{
		perror ("Error: send length = 0 as done");
		//close files
		close (input_ciphertext_fd);
		close (input_key_fd);
		exit(2);
	}

	//close files
	close (input_ciphertext_fd);
	close (input_key_fd);

	close(socketFD); // Close the socket

	exit(0);
}
