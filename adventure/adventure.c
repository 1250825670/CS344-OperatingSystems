/***********************************************************************************
Author: Siara Leininger
Date: May 5, 2017
Class: CS 344
Assignment: Program 2 - Adventure
The program read 7 different room files, one room per file, in a
directory called "<STUDENT ONID USERNAME>.rooms.<PROCESS ID>
then display room to allow user to move between room until the end room is found
The game also displays the steps to move and path from start room to end room
References:
http://home.hccnet.nl/r.helderman/adventures/htpataic01.html
http://forum.devmaster.net/t/linking-rooms-in-text-based-game-c/19208
https://github.com/Bazze/Colossal-Cave-Adventure-C-plus-plus
http://stackoverflow.com/questions/14888027/mutex-lock-threads
https://baptiste-wicht.com/posts/2012/03/cp11-concurrency-tutorial-part-2-protect-shared-data.html
http://www.thegeekstuff.com/2012/05/c-mutex-examples/?refcom
https://www.tutorialspoint.com/c_standard_library/c_function_fgets.htm
http://www.dummies.com/programming/c/how-to-use-the-fgets-function-for-text-input-in-c-programming/
*************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

//define maximum outgoing connections
#define MAX_NUMBER_CONNECTIONS 6

//build room program
#define BUILD_ROOM_PROGRAM "leinings.buildrooms"

//maximum character in a string
#define STRING_LENGTH 101

//number of file generated
#define NUM_FILES 7

//maximum number of rooms in path
#define NUM_ROOMS_PATH 100

//current time
#define CURRENT_TIME "currentTime.txt"

//pthread mutex
static pthread_mutex_t pthread_mutex_access;

//create a condition variable
static pthread_cond_t ok = PTHREAD_COND_INITIALIZER;

/**********************************************************
share variable
requested time = 0: no request, 1: has reqest; -1, exit
************************************************************/
static int requestedTime = 0;

//define room structures
struct ROOM_STRUCT{
	char roomName[STRING_LENGTH]; //room name
	char connections[MAX_NUMBER_CONNECTIONS][STRING_LENGTH];//connections
	int numConnections;//number of connections
	char roomType[STRING_LENGTH]; //room type
};

/**********************
read rooms from file
************************/
void readRooms(struct ROOM_STRUCT rooms[], char dirFilename[], char filenames[][STRING_LENGTH]){

	int i, j; //loop control variables

	FILE *fp; //file pointer	

	char line[STRING_LENGTH]; //a line from file

	//file name
	char filename[STRING_LENGTH];

	//connection
	char connection[STRING_LENGTH];

	//initialize rooms
	for (i = 0; i < NUM_FILES; i++)
	{
		rooms[i].numConnections = 0;
	}

	//read files
	for (i = 0; i < NUM_FILES; i++){

		//create file name
		sprintf(filename, "%s/%s", dirFilename, filenames[i]);		
		fp = fopen(filename, "r");

		//read file
		fgets(line, STRING_LENGTH, fp);
		sscanf(line, "ROOM NAME: %[^\n]\n", rooms[i].roomName);

		while (1)
		{//read connnection until room type
			fgets(line, STRING_LENGTH, fp);
			if (strncmp(line, "CONNECTION", 10) == 0)
			{
				sscanf(line, "CONNECTION %d: %[^\n]\n", &j, connection);
				strcpy(rooms[i].connections[rooms[i].numConnections], connection);
				rooms[i].numConnections++;
			}else{//room type
				break;
			}
		}
		sscanf(line, "ROOM TYPE: %[^\n]\n", rooms[i].roomType);

		//close file
		fclose(fp);
	}
}

/**************************************
get room index in array by room name
from current room index
return -1 if not found
***************************************/
int getRoomIndex(struct ROOM_STRUCT rooms[], int currentRoomIndex, char roomName[]){
	
	int i; //loop control variable
	int found = 0; //if there is direct path between 2 rooms

	//check if there is path from current room to room name
	for (i = 0; i < rooms[currentRoomIndex].numConnections; i++)
	{
		if (strcmp(rooms[currentRoomIndex].connections[i], roomName) == 0)
		{
			found = 1;
		}
	}

	//if path is found
	if (found == 1)
	{
		//find next room
		for (i = 0; i < NUM_FILES; i++)
		{
			if (strcmp(rooms[i].roomName, roomName) == 0)
			{
				return i;
			}
		}
	}

	
	return -1;//not found
}

/**********************************
get start room index
precondition: start room in array
***********************************/
int getStartRoomIndex(struct ROOM_STRUCT rooms[]){
	
	int i; //loop control variable
	for (i = 0; i < NUM_FILES; i++)
	{
		if (strcmp(rooms[i].roomType, "START_ROOM") == 0)
		{
			return i;
		}
	}
	return -1;//not here
}

/************************************************
run game
start from start room
display room and allow player to move
the system keep track the path, number of steps
game exit when the room is end room
**************************************************/
void runGame(struct ROOM_STRUCT rooms[]){
	
	int i; //loop control variable
	int numSteps = 0; //number of step to move to end room

	//get start room index
	int currentRoomIndex = getStartRoomIndex(rooms);
	//path
	char path[NUM_ROOMS_PATH][STRING_LENGTH];
	int pathLength = 0; //rooms in path

	//next room index
	int nextRoomIndex;

	char roomName[STRING_LENGTH]; //room name
	
	//add to start room to path
	strcpy(path[pathLength++], rooms[currentRoomIndex].roomName); 

	//run until the room is end room
	while (strcmp(rooms[currentRoomIndex].roomType, "END_ROOM") != 0)
	{
		printf("CURRENT LOCATION: %s\n", rooms[currentRoomIndex].roomName);
		printf("POSSIBLE CONNECTIONS: ");
		for (i = 0; i < rooms[currentRoomIndex].numConnections; i++)
		{
			if (i > 0)
			{
				printf(", ");
			}
			printf("%s", rooms[currentRoomIndex].connections[i]);
		}
		printf(".\nWHERE TO? >");

		//read a line
		fgets(roomName, STRING_LENGTH, stdin);
		//remove new line
		roomName[strlen(roomName) - 1] = '\0';

		printf("\n");

		if (strcmp(roomName, "time") == 0)
		{
			//printf("TIME Request\n");

			//request a lock
			pthread_mutex_lock(&pthread_mutex_access);
			requestedTime = 1; //print time
			pthread_cond_signal(&ok); //notify child thread to print time			
			pthread_mutex_unlock(&pthread_mutex_access);
			
			//wait for printing time
			pthread_mutex_lock(&pthread_mutex_access);
			while (requestedTime == 1)
			{
				pthread_cond_wait(&ok, &pthread_mutex_access);
			}
			pthread_mutex_unlock(&pthread_mutex_access);

		}else{ //move to next room

			nextRoomIndex = getRoomIndex(rooms, currentRoomIndex, roomName);
			if (nextRoomIndex == -1)
			{
				printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
			}else{
				currentRoomIndex = nextRoomIndex;

				//add to path and increase steps
				strcpy(path[pathLength++], roomName); 
				numSteps++;
			}
		}
	}//end while

	printf("YOU HAVE REACHED THE FINAL ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", numSteps);
	for (i = 0; i < pathLength; i++)
	{
		printf("%s\n", path[i]);
	}
}

//job run by thread
void *timeTask(void *arg) {

	time_t timer;
    char buffer[STRING_LENGTH];
    struct tm* tm_info;

	//FILE pointer
	FILE *pTime;

	//if task is done
	int done = 0;

	//printf("\nChild: RUNNING\n\n");

	while (done == 0)
	{
		pthread_mutex_lock(&pthread_mutex_access);

		//printf("\nChild: WAITING\n\n");

		while (requestedTime == 0)
		{
			pthread_cond_wait(&ok, &pthread_mutex_access);
		}

		//printf("\nChild: GOT lock requestedTime = %d\n\n", requestedTime);

		//requested time
		if (requestedTime == 1)
		{
			//retrieve current time and display time
			//e.g 1:03pm, Tuesday, September 13, 2016
			time(&timer);
			tm_info = localtime(&timer);
			strftime(buffer, STRING_LENGTH, "%I:%M%p, %A, %B %d, %Y", tm_info);
			printf("\n%s\n\n\n", buffer);

			pTime = fopen(CURRENT_TIME, "a");
			if (pTime == NULL)
			{
				perror("Could not open file for write time.\n");
				exit(EXIT_FAILURE);
			}

			//write time and close file
			fprintf(pTime, "%s\n", buffer);
			fclose(pTime);

			requestedTime = 0; //printed time
		}

		//exit request
		if (requestedTime == -1)
		{
			//printf("\nDONE GOT\n\n");
			done = 1;
		}
		pthread_cond_signal(&ok); //notify parent thread to continue	
		pthread_mutex_unlock(&pthread_mutex_access);
	}
}

/****************************************************
retrieve the data (rooms) directory with most time
*****************************************************/
void retriveDataFolder(char dirFilename[]){

	DIR *dp;
	struct dirent *ep;
	struct stat attrib;
	time_t mostTime = 0;

	//printf("retriveDataFolder mostTime = %ld\n", mostTime);

	dp = opendir (".");
	if (dp != NULL)
	{
		//read files or folders in current directory
		while (ep = readdir (dp)){
			
			stat(ep->d_name, &attrib);

			//printf("%s, attrib.st_ctime = %ld\n", ep->d_name, attrib.st_ctime);
			//printf("S_ISDIR(attrib.st_mode) = %d\n", S_ISDIR(attrib.st_mode));

			if (strcmp(ep->d_name, ".") != 0 && strcmp(ep->d_name, "..") != 0 && 
				S_ISDIR(attrib.st_mode) && attrib.st_ctime > mostTime)
			{
				strcpy(dirFilename, ep->d_name);
				mostTime = attrib.st_ctime;
			}		
		}

		(void) closedir (dp);
	}
	else{
		perror ("Could not open the directory.");
		exit(EXIT_FAILURE);
	}
}
int main(){

	struct ROOM_STRUCT rooms[NUM_FILES]; //array of rooms

	//directory name
	char dirFilename[STRING_LENGTH];

	//room file names
	char roomFilenames[NUM_FILES][STRING_LENGTH];

	int err; //error

	//thread id
	pthread_t child;

	//set file names
	strcpy(roomFilenames[0], "file1.txt");
	strcpy(roomFilenames[1], "file2.txt");
	strcpy(roomFilenames[2], "file3.txt");
	strcpy(roomFilenames[3], "file4.txt");
	strcpy(roomFilenames[4], "file5.txt");
	strcpy(roomFilenames[5], "file6.txt");
	strcpy(roomFilenames[6], "file7.txt");

	//retrieved directory
	retriveDataFolder(dirFilename);

	//printf("Directory is %s\n", dirFilename);

	//create a mutex, mutex will be automatically initialized to one
	pthread_mutex_init(&pthread_mutex_access, NULL);

	//fork a different child process 
	err = pthread_create(&child, NULL, timeTask, NULL);

	if (err)
	{
		perror("Error: could not create child thread\n");
		exit(EXIT_FAILURE);
	}

	//read rooms structure
	readRooms(rooms, dirFilename, roomFilenames);

	//play game
	runGame(rooms);

	//lock mutex, notify to exit
	pthread_mutex_lock(&pthread_mutex_access);
	requestedTime = -1; //exit request
	pthread_cond_signal(&ok); //notify child thread to exit
	pthread_mutex_unlock(&pthread_mutex_access);

	//wait for thread
	pthread_join(child, NULL);

	return 0;
}
