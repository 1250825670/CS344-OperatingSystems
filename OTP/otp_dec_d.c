*
File otp_enc_d.c
The program works are multithreading TCP Socket server (using fork)
that decode the ciphertext based on provided key
using modulo 27 where 27 characters are the 26 capital letters, and the space character.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFF_SIZE 256 //buffer size
#define LENGTH_SIZE 10 //10 characters notifying about length of data
#define MAX_CONNECTIONS 5 //max socket connect
#define CLIENT_PROGRAM "otp_dec"
void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues, prints it to stderr 
    
/*
decode the ciphertext by subtracting character to key one by one then modulo 27
precondition: plaintext has enough memory to store the result. 
              The length of key is greater than or equal length of ciphertext
postcondition: plaintext contains decrypted text
return 1 if success, 0 if false
*/
int decode(char *ciphertext, char *key, char *plaintext){
    char* STRING = " ABCDEFGHIJKLMNOPQRSTUVWXYZ"; //valid characters
	int index;//index of decrypted character
	unsigned int i; //loop control variable	

    for (i = 0; i < strlen(ciphertext); i++){

		//validate, character must be space or A-Z
		if (ciphertext[i] != ' ' && !(ciphertext[i] >= 'A' && ciphertext[i] <= 'Z'))
		{
			//printf("Debug: ciphertext[i] = %u %c\n", ciphertext[i], ciphertext[i]);
			return 0;
		}

		index  = 0;
		if (ciphertext[i] == ' '){
			index = -key[i];
		}else{
			index = (ciphertext[i] - 'A' + 1 - key[i]);
		}

		//add 27 if it is negative
		while (index < 0){
			index += 27;
		}

		//modulo
		index = index % 27;

		//retrieve and set value of decrypted character
		plaintext[i] = STRING[index];
    }
    return 1; //return sucess
}

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
		if (charsRead < 0) error("Error: reading from socket");

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
		return 0;
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
		if (charsRead < 0) error("Error: reading from socket");

		if (charsRead == 0){
			break;
		}

		totalCharReceived += charsRead;

		buffer[charsRead] = '\0'; //append with '\0'
		strcat(data, buffer); //appen to bufferLength
	}

	//check if it receives enough number of characters
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
	}

	//check if it sent enough number of characters
	if (totalCharSent != LENGTH_SIZE){
		error("Error: send length to socket");
	}

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
		if (charsWrote < 0) error("ERROR writing to socket");

		if (charsWrote == 0){
			break;
		}

		totalCharSent += charsWrote;
		bufferPointer += charsWrote;

		//printf("Debug: sent totalCharSent = %d\n", totalCharSent);
	}
	
	return length;
}

//main function start C program
int main(int argc, char *argv[]){
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char plaintext[BUFF_SIZE];
	char key[BUFF_SIZE];
	char ciphertext[BUFF_SIZE];
	pid_t childpid;
	int ret_receive_key, ret_receive_ciphertext, ret_decode, ret_sent, ret_in; //characters received/sent
	char program[BUFF_SIZE]; //program name

	struct sockaddr_in serverAddress, clientAddress;
	int totalCharReceived, totalCharSent; //total number of characters received/sent

	if (argc != 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, MAX_CONNECTIONS); // Flip the socket on - it can now receive up to MAX_CONNECTIONS connections

	for ( ; ; ) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

		//printf("Debug: got connection\n");

		if ( (childpid = fork ()) == 0 ) {// child process

			//close listening socket
			close (listenSocketFD);

			//read program name
			ret_in = receiveData(program, establishedConnectionFD);

			//printf("Debug: got program: %s length = %d\n", program, ret_in);

			if (strcmp(program, CLIENT_PROGRAM) != 0)
			{
				strcpy(program, "ERROR");
				ret_sent = sendData(program, establishedConnectionFD);

				fprintf(stderr,"Error: invalid client, expected %s\n", CLIENT_PROGRAM); 
				exit(1);
			}else{
				strcpy(program, CLIENT_PROGRAM);
				ret_sent = sendData(program, establishedConnectionFD);
			}

			
			//read plain text
			while ((ret_receive_ciphertext = receiveData(ciphertext, establishedConnectionFD)) > 0){

				//printf("Debug: got ciphertext: %s length = %d\n", ciphertext, ret_receive_ciphertext);

				//read key
				ret_receive_key = receiveData(key, establishedConnectionFD);

				//printf("Debug: got key: %s length = %d\n", key, ret_receive_key);

				if (ret_receive_ciphertext != ret_receive_key)
				{
					fprintf(stderr,"Error: lengths of ciphertext and key are different, %d %d\n", ret_receive_ciphertext, ret_receive_key); 
					exit(1);
				}

				//encrypt
				memset(plaintext, '\0', BUFF_SIZE);
				ret_decode = decode(ciphertext, key, plaintext);

				if (ret_decode == 0)
				{
					fprintf(stderr,"Error: decode code error\n"); 
					exit(1);
				}

				//printf("Debug: plaintext: %s length = %d\n", plaintext, ret_decode);

				//send the client
				ret_sent = sendData(plaintext, establishedConnectionFD);

				//printf("Debug: sent plaintext text: %s length = %d\n", plaintext, ret_sent);
			}

			exit(0);		
		}

		//parent continue
		//close socket of the server
		close(establishedConnectionFD);	 
	}

	close(listenSocketFD); // Close the listening socket

	return 0; 
}
