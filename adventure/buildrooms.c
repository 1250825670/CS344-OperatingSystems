/********************************************************************
Author: Siara Leininger
Date: May 5, 2017
Class: CS 344
Assignment: Program 2 - Adventure
The program generates 7 different room files, one room per file, in a
directory called "<STUDENT ONID USERNAME>.rooms.<PROCESS ID>
References: 
http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
http://stackoverflow.com/questions/11291154/save-file-listing-into-array-or-something-else-c
http://stackoverflow.com/questions/19117131/get-list-of-file-names-and-store-them-in-array-on-linux-using-c
http://stackoverflow.com/questions/10446526/get-last-modified-time-of-file-in-linux
http://stackoverflow.com/questions/1674032/static-const-vs-define-vs-enum
https://www.tutorialspoint.com/c_standard_library/c_function_sprintf.htm
https://www.tutorialspoint.com/c_standard_library/c_function_fprintf.htm
https://www.tutorialspoint.com/c_standard_library/c_function_fopen.htm
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <sys/types.h> //process id
#include <unistd.h>

//fixed username
#define STUDENT_ONID_USERNAME "leinings"

//maximum character in a string
#define STRING_LENGTH 101
//number of room names
#define NUM_NAMES 10
//number of file generated
#define NUM_FILES 7

//define mininum/maximum outgoing connections
#define MIN_NUMBER_CONNECTIONS 3
#define MAX_NUMBER_CONNECTIONS 6

//number of room types
#define NUMBER_ROOM_TYPES 3

//define room structures
struct ROOM_STRUCT{
	int roomNameIndex; //room name index
	int connections[MAX_NUMBER_CONNECTIONS];//connections (room indices)
	int numConnections;//number of connections
	int roomTypeIndex; //room type index
};

/****************************************************
get random room name
return the room name index (int* roomNameIndex)
rooms: array of structures
roomNames: array of room names
roomNameIndex: random index chosen
roomIndex: the number of structs set so far
*******************************************************/
void randomRoomNameIndex(struct ROOM_STRUCT rooms[], char roomNames[][STRING_LENGTH], int* roomNameIndex, int roomIndex){

	int i; //loop control variable
	int duplicate = 1; //duplicated names

	while (duplicate == 1)
	{
		duplicate = 0;

		*roomNameIndex = rand() % NUM_NAMES; //random index

		//check if room name has been used
		for (i = 0; i < roomIndex; i++)
		{
			if (rooms[i].roomNameIndex == *roomNameIndex)
			{
				duplicate = 1;
				break;
			}
		}
	}
}

/*****************************************************
add connections to rooms
precondition: fromRoom, toRoom are valid indices
return 1 if success
********************************************************/
int addConnection(struct ROOM_STRUCT rooms[], int fromRoom, int toRoom){
	int i; //loop control variable
	if (rooms[fromRoom].numConnections < MAX_NUMBER_CONNECTIONS && rooms[toRoom].numConnections < MAX_NUMBER_CONNECTIONS)
	{
		//check if toRoom is connection already
		for (i = 0; i < rooms[fromRoom].numConnections; i++)
		{
			if (rooms[fromRoom].connections[i] == toRoom)
			{
				return 0;
			}
		}
		rooms[fromRoom].connections[rooms[fromRoom].numConnections++] = toRoom;
		rooms[toRoom].connections[rooms[toRoom].numConnections++] = fromRoom;
		return 1;
	}
	return 0;
}

/****************************
choose random room types
*******************************/
void chooseRandomRoomTypes(struct ROOM_STRUCT rooms[]){

	int i; //loop control variable
	int startRoomIndex, endRoomIndex; //start, end room indices

	//choose random start room
	startRoomIndex = rand() % NUM_FILES;
	rooms[startRoomIndex].roomTypeIndex = 0;

	//create random end room
	endRoomIndex = rand() % NUM_FILES;
	while (startRoomIndex == endRoomIndex)
	{
		endRoomIndex = rand() % NUM_FILES;
	}
	rooms[endRoomIndex].roomTypeIndex = 1;

	//set other rooms to MID_ROOM
	for (i = 0; i < NUM_FILES; i++)
	{			
		if (i != startRoomIndex && i != endRoomIndex)
		{
			rooms[i].roomTypeIndex = 2;
		}
	}
}

/*************************************
                 MAIN
**************************************/
int main(){
	
	//room names
	char roomNames[NUM_NAMES][STRING_LENGTH];

	//room types
	char roomTypes[NUMBER_ROOM_TYPES][STRING_LENGTH];

	//room file names
	char roomFilenames[NUM_FILES][STRING_LENGTH];

	//directory name
	char dirFilename[STRING_LENGTH];

	//file name
	char filename[STRING_LENGTH];

	int i, j, k; //loop control variables

	FILE *fp; //file pointer

	struct ROOM_STRUCT rooms[NUM_FILES]; //array of rooms

	int index; //random index

	int numConnections; //number of random connections

	srand(time(0)); //initialize random number generator

	//initialize rooms
	for (i = 0; i < NUM_FILES; i++)
	{
		rooms[i].numConnections = 0;
	}

	//set names
	strcpy(roomNames[0], "Ravenclaw");
	strcpy(roomNames[1], "Hufflepuff");
	strcpy(roomNames[2], "Gryffindor");
	strcpy(roomNames[3], "Slytherin");
	strcpy(roomNames[4], "RoomOfRequirement");
	strcpy(roomNames[5], "ForbiddenForest");
	strcpy(roomNames[6], "Ollivanders");
	strcpy(roomNames[7], "HeadmasterOffice");
	strcpy(roomNames[8], "Library");
	strcpy(roomNames[9], "Gringotts");	

	//set file names
	strcpy(roomFilenames[0], "file1.txt");
	strcpy(roomFilenames[1], "file2.txt");
	strcpy(roomFilenames[2], "file3.txt");
	strcpy(roomFilenames[3], "file4.txt");
	strcpy(roomFilenames[4], "file5.txt");
	strcpy(roomFilenames[5], "file6.txt");
	strcpy(roomFilenames[6], "file7.txt");

	//set room types
	strcpy(roomTypes[0], "START_ROOM");	
	strcpy(roomTypes[1], "END_ROOM");	
	strcpy(roomTypes[2], "MID_ROOM");	
	
	//create directory
	//strcpy(dirFilename, "mydir");
	sprintf(dirFilename, "%s.rooms.%d", STUDENT_ONID_USERNAME, getpid());
	
	if(mkdir(dirFilename, 0755) != -1){

		//create random room names
		for (i = 0; i < NUM_FILES; i++)
		{
			//get random room name
			randomRoomNameIndex(rooms, roomNames, &rooms[i].roomNameIndex, i);			
		}

		//set random connections
		for (i = 0; i < NUM_FILES; i++)
		{
			if (rooms[i].numConnections < MIN_NUMBER_CONNECTIONS)
			{
				//generate random number of connections
				numConnections = rand() % (MAX_NUMBER_CONNECTIONS - MIN_NUMBER_CONNECTIONS + 1) + MIN_NUMBER_CONNECTIONS;

				//add connections
				j = rooms[i].numConnections;
				while (j < numConnections)
				{
					index = rand() % NUM_FILES;
					//check if same rooms or selected room
					while(i == index || addConnection(rooms, i, index) == 0){
						index = rand() % NUM_FILES;
					}
					j++;
				}
			}			
		}

		//choose random room types
		chooseRandomRoomTypes(rooms);

		//create files
		for (i = 0; i < NUM_FILES; i++)
		{
			//create file name
			sprintf(filename, "%s/%s", dirFilename, roomFilenames[i]);		
			fp = fopen(filename, "w+");

			//write content
			fprintf(fp, "ROOM NAME: %s\n",  roomNames[rooms[i].roomNameIndex]);
			for (j = 0; j < rooms[i].numConnections; j++)
			{
				fprintf(fp, "CONNECTION %d: %s\n", j + 1, roomNames[rooms[rooms[i].connections[j]].roomNameIndex]);
			}
			fprintf(fp, "ROOM TYPE: %s\n", roomTypes[rooms[i].roomTypeIndex]);

			//close file
			fclose(fp);
		}

	}else{
		printf("Could not create directory\n");
		return -1;
	}	

	return 0;
}
