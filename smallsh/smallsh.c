***********************************************************************
Author: Siara Leininger
Date: May 18, 2017
Class: CS 344
Assigment: Program 3 - smallsh
References: //http://stackoverflow.com/questions/33848558/catching-sigterm-in-c
http://stackoverflow.com/questions/4788374/writing-a-basic-shell
http://codewiki.wikidot.com/c:system-calls:dup2
http://www.linuxforums.org/forum/programming-scripting/40078-c-printing-linux-process-table.html
http://c.happycodings.com/c-on-unix/code14.html
http://www.includehelp.com/c-programs/get-process-parent-id-in-linux.aspx
https://stackoverflow.com/questions/9642732/parsing-command-line-arguments
https://stackoverflow.com/questions/41230547/check-if-program-is-installed-in-c
https://www.tutorialspoint.com/c_standard_library/c_function_getenv.htm
https://stackoverflow.com/questions/27998351/what-would-wifexitedstatus-be-when-a-process-just-ran-the-command-true
http://pubs.opengroup.org/onlinepubs/009695399/functions/chdir.html
https://stackoverflow.com/questions/17096990/why-use-bzero-over-memset
*************************************************************************/

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define true 1
#define false 0

//command lines with a maximum length of 2048 characters
#define MAX_LENGTH_CMD_LINE 2048 + 1//last character is '\0'
//a maximum of 512 arguments
#define MAX_ARGUMENTS 312 + 1 //last argument is set to NULL
// background processes size
#define BACKGROUND_PROCESSES_DEFAULT_SIZE 100
//buffer size
#define BUF_SIZE 101

//PIDs of noncompleted background processes in an array
pid_t* PIDs = NULL;
//PIDs array size 
int PIDsArraySize = BACKGROUND_PROCESSES_DEFAULT_SIZE;
int PIDSize = 0; //current elements in array
int acceptBackground = true; //allow background process
int exitSignal = 0; //signal number when exited

//replace $$ in text by pid
void replaceStr(char text[]){
	int i, j; //loop control variable
	char prefix[BUF_SIZE]; //string before $$
	char posfix[BUF_SIZE]; //string after $$

	//find the position of $$
	for (i = 0; i < strlen(text) - 1; i++)
	{
		if (text[i] == '$' && text[i + 1] == '$')
		{
			break;
		}
	}

	if (i < strlen(text) - 1)
	{//found $$ at i
		strncpy(prefix, text, i);
		prefix[i] = '\0';
		strncpy(posfix, text + i + 2, strlen(text) - i - 2);
		posfix[strlen(text) - i - 2] = '\0';
		sprintf(text, "%s%d%s", prefix, getpid(), posfix);
	}
}

//parse command
void parseCommand(char command[], char program[], char arguments[][MAX_LENGTH_CMD_LINE], char inputRedirect[],  char outputRedirect[], char* background, int* numArguments){

	//a token in command
	char * pch;
	*numArguments = 0; //arguments

	//set default value
	strcpy(inputRedirect, "");
	strcpy(outputRedirect, "");
	*background = '\0';
	
	if (strlen(command) > 0 && command[0] != '#'){
		pch = strtok (command, " ");
	
		//first token is program and first arguments
		strcpy(program, pch);
		strcpy(arguments[(*numArguments)++], pch);

		//next token
		pch = strtok (NULL, " ");
		while (pch != NULL)
		{
			//input redirection
			if (strcmp(pch, "<") == 0){
				pch = strtok (NULL, " ");
				strcpy(inputRedirect, pch);
			}else if (strcmp(pch, ">") == 0){
				pch = strtok (NULL, " ");
				strcpy(outputRedirect, pch);
			}else if (strcmp(pch, "&") == 0){
				*background = '&';
			}else{//arguments
				
				//replace $$ in text by pid
				replaceStr(pch);

				strcpy(arguments[(*numArguments)++], pch);
			}

			//next token
			pch = strtok (NULL, " ");
		}

	}else{//comment only
		strcpy(program, command);
	}
}

//put pid_t to array
pid_t* putPid (pid_t pid){
	pid_t* temp; //new array
	int i; //loop control variable
	if (PIDsArraySize == PIDSize)
	{
		//extends array
		(PIDsArraySize) *= 2;
		temp = (pid_t*)malloc((PIDsArraySize) * sizeof(pid_t));
		for (i = 0; i < PIDSize; i++)
		{
			temp[i] = PIDs[i];
		}
		free (PIDs);
		PIDs = temp;
	}

	PIDs[(PIDSize)++] = pid;
	return PIDs;
}

//check if program in path
int inPath(char program[]){

	char* pPath; //path
	//a token in command
	char * pch;
	char * pathBuffer; //path buffer

	//for loop the directory to find the program
	DIR *dp;
	struct dirent *ep;

	int found = false;

	//read PATH enviroment
	pPath = getenv ("PATH");

	//printf("DEBUG: PATH = %s\n", pPath);

	pathBuffer = (char*)malloc((strlen(pPath) + 1) * sizeof(char));
	strcpy(pathBuffer, pPath);

	pch = strtok (pathBuffer, ":");
	while (found == false && pch != NULL)
	{
		//printf("Open directory %s\n", pch);
		dp = opendir (pch);
		if (dp != NULL)
		{
			//loop directory in PATH
			while (found == false && (ep = readdir (dp)) != NULL){
				//printf("%s/%s\n", pch, ep->d_name);
				//compare file with program
				if (strcmp(ep->d_name, program) == 0)
				{
					found = true;
				}
			}

			//close directory
			(void) closedir (dp);
		}

		
		//next token
		pch = strtok (NULL, ":");
	}//end for one directory

	free (pathBuffer);

	return found;
}

//check if background processes have been terminated
void checkBackgroundProcessTermination(){
	int i, j; //loop control variable
	int childExitMethod = -5;
	int childPID_actual; //return value of waitpid
	int exitStatus;//exit status
	int termSignal;//signal
	int hasTerminate = true;

	//check until no process is completed
	while (hasTerminate){

		hasTerminate = false;
		for (i = 0; i < PIDSize; i++)
		{
			childPID_actual = waitpid(PIDs[i], &childExitMethod, WNOHANG);

			if (childPID_actual > 0) //this process with pid completed
			{
				if (WIFEXITED(childExitMethod))
				{
					exitStatus = WEXITSTATUS(childExitMethod);
					printf("background pid %d is done: exit value %d\n", PIDs[i], exitStatus);
				}
				else{
					termSignal = WTERMSIG(childExitMethod);
					printf("background pid %d terminated by signal %d\n", PIDs[i], termSignal);
				}

				fflush(stdout);

				hasTerminate = true;

				//delete pid in array
				for (j = i; j < (PIDSize) - 1; j++)
				{
					PIDs[j] = PIDs[j + 1];
					(PIDSize)--;
				}
			}
		}	
	}//end while
}

//process command
//return exit status
int processCommand(char program[], char arguments[][MAX_LENGTH_CMD_LINE], char inputRedirect[],  
				   char outputRedirect[], char background, int numArguments, int* status){
	
	pid_t spawnpid = -5; //spawn proces id
	pid_t actualPid; //return value method by waitpid
	int childExitStatus = -5;//exit status
	char* programArguments[MAX_ARGUMENTS]; //program argument in calling to exec...function
	int i; //loop control variable
	char path[MAX_LENGTH_CMD_LINE];//path in cd
	char* pathPointer; //pointer return from getcwd
	char buf[MAX_LENGTH_CMD_LINE]; //error buffer
	int programExitStatus; //program exit status
	int targetFD; //target file descriptor
	int sourceFD; //source file descriptor
	int resultDup; //dup2 return value
	int exitStatus;

	if (strlen(program) == 0 || program[0] == '#')
	{//empty command or comment only
		//ignore
		return true; //for continue to run shell
	}

	//exit, cd, and status
	if (strcmp(program, "exit") == 0)
	{		
		//kill any other processes or jobs		

		int i; //loop control variable
		pid_t actualPid; //return value method by waitpid
		int childExitMethod = -5;

		//printf("DEBUG: catchSIGINT is called\n");
		//fflush(stdout);

		for (i = 0; i < PIDSize; i++)
		{
			kill(PIDs[i], SIGKILL);
		
		}

		for (i = 0; i < PIDSize; i++)
		{
			//wait for child process to exit
			actualPid = waitpid(PIDs[i], &childExitMethod, 0);			
		}

		PIDSize = 0;//all background processes completed

		return false;
	}else if (strcmp(program, "cd") == 0)// changes directories
	{
		//support both absolute and relative paths
		//Sets the current working directory
		//printf("DEBUG: %s\n", arguments[1]);
		if (arguments[1][0] != '/') //relative paths
		{
			pathPointer = getcwd(buf, MAX_LENGTH_CMD_LINE);
			if (pathPointer == NULL)
			{
				perror("Could not read current working directory\n");
				fflush(stdout);
				return false;
			}else{
				strcpy(path, pathPointer);
				strcat(path, "/");
				strcat(path, arguments[1]);
			}
		}else{
			strcpy(path, arguments[1]);
		}
		if (chdir(path) != 0){
			perror("Could not change current working directory\n");
			fflush(stdout);
		}
	}else if (strcmp(program, "status") == 0)// view status
	{
		if (background == '&' && exitSignal != 0)
		{
			printf("terminated by signal %d\n", exitSignal);
		}else{
			printf("exit value %d\n", *status);
		}
		fflush(stdout);
	}else{

		//use the PATH variable to look for non-built in commands,
		//and it should allow shell scripts to be executed. If a command fails because
		//the shell could not find the command to run, then the shell will print an error
		//message and set the exit status to 1
		if (inPath(program) == false){
			printf("%s: no such file or directory\n", program);
			fflush(stdout);
			*status = 1;
		}else{		

			//check if background process allowed
			if (acceptBackground == false)
			{
				background = '\0';
			}

			//printf("DEBUG: fork, exec...\n");
			//copy arguments
			for (i = 0; i < numArguments; i++)
			{
				programArguments[i] = arguments[i];
			}
			programArguments[numArguments] = NULL;
			
			spawnpid = fork();
			switch (spawnpid)
			{
			case -1:
				perror("Hull Breach!");
				exit(1);
				break;
			case 0: //child

				//rediect input
				if (strlen(inputRedirect) > 0 || (strlen(inputRedirect) == 0 && background == '&')){

					//Background commands should have their standard input redirected from
					///dev/null if the user did not specify some other file to take standard input
					//from.
					if (strlen(inputRedirect) == 0 && background == '&')
					{
						strcpy(inputRedirect, "/dev/null");
					}
					sourceFD = open(inputRedirect, O_RDONLY);
					if (sourceFD == -1) { 
						printf("cannot open %s for input\n", inputRedirect);
						fflush(stdout);
						exit(1);
					}
						
					resultDup = dup2(sourceFD, 0); //redirect stdin to targetFD
					if (resultDup == -1) { 
						printf("cannot call dup2 for input\n");
						fflush(stdout);
						exit(1);
					}

					//Close file on Exec
					fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
				}

				//redirect output
				if (strlen(outputRedirect) > 0 || (strlen(outputRedirect) == 0 && background == '&')){

					//Background commands should also not send their
					//standard output to the screen (again, redirect to /dev/null).
					if (strlen(outputRedirect) == 0 && background == '&')
					{
						strcpy(outputRedirect, "/dev/null");
					}

					targetFD = open(outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (targetFD == -1) { 
						printf("cannot open %s for output\n", outputRedirect);
						fflush(stdout);
						exit(1);
					}
						
					resultDup = dup2(targetFD, 1); //redirect stdout to targetFD
					if (resultDup == -1) { 
						printf("cannot call dup2 for output\n");
						fflush(stdout);
						exit(1);
					}

					//Close file on Exec
					fcntl(targetFD, F_SETFD, FD_CLOEXEC);
				}

				//printf("program = %s\n", program);
				//printf("programArguments[0] = %s\n", programArguments[0]);
				//printf("programArguments[1] = %s\n", programArguments[1]);

				//read PATH enviroment
				//char* pPath = getenv ("PATH");
				//printf("DEBUG: PATH = %s\n", pPath);
					
				*status = 0;
				if (execvp(program, programArguments) < 0)
				{
					printf("Command could not be executed!\n");
					fflush(stdout);
					*status = 1;
					exit(1);
				}
				break;
			default: //parent
				if (background == '&'){//back ground process
					//The shell will print the process id of a background process when it begins
					printf("background pid is %d\n", spawnpid);
					fflush(stdout);

					//put pid_t to array
					PIDs =  putPid(spawnpid);

				}else{

					//wait for child process to exit
					actualPid = waitpid(spawnpid, &childExitStatus, 0);

					if (actualPid == -1)
					{
						//ignore (signal)
					}else {

						if (WIFEXITED(childExitStatus))
						{//exit normally
							exitStatus = WEXITSTATUS(childExitStatus);
							if (exitStatus != 0)
							{
								*status = 1;
							}else{
								*status = 0;
							}

							//printf("DEBUG: process exits normally\n");
							fflush(stdout);
						}
					}
				}
				break;
			}
		}//end check PATH
	}
	return true; //for continue to run shell
}


//catch SIGINT signal
//SIGINT is also sent to child processes
//the parent process waits for child processes and print message
void catchSIGINT(int signo)
{
	int i; //loop control variable
	pid_t actualPid; //return value method by waitpid
	int exitStatus;//exit status
	int termSignal;//signal
	int childExitMethod = -5;
	char buffer[BUF_SIZE];

	exitSignal = signo;

	//printf("DEBUG: catchSIGINT is called\n");
	//fflush(stdout);
	sprintf(buffer, "terminated by signal %d\n", signo);
	write(STDOUT_FILENO, buffer, strlen(buffer));

	for (i = 0; i < PIDSize; i++)
	{
		//wait for child process to exit
		actualPid = waitpid(PIDs[i], &childExitMethod, 0);

		if (actualPid == -1)
		{
			//ignore
			//printf("DEBUG: catchSIGINT is called, waitpid = -1\n");
			//fflush(stdout);
		}else{

			if (WIFEXITED(childExitMethod))
			{
				exitStatus = WEXITSTATUS(childExitMethod);

				sprintf(buffer, "background pid %d is done: exit value %d\n", PIDs[i], exitStatus);
				write(STDOUT_FILENO, buffer, strlen(buffer));

			}
			else{
				termSignal = WTERMSIG(childExitMethod);

				sprintf(buffer, "background pid %d is done: terminated by signal %d\n", PIDs[i], termSignal);
				write(STDOUT_FILENO, buffer, strlen(buffer));
			}
		}
	}

	PIDSize = 0;//all background processes completed
}

//catch SIGTSTP signal
//change allow or disallow background
void catchSIGTSTP(int signo)
{

	char buffer[BUF_SIZE];

	if (acceptBackground == true)
	{
		acceptBackground = false;		

		sprintf(buffer, "Entering foreground-only mode (& is now ignored)\n");
		write(STDOUT_FILENO, buffer, strlen(buffer));

	}else{

		sprintf(buffer, "Exiting foreground-only mode\n");
		write(STDOUT_FILENO, buffer, strlen(buffer));

		acceptBackground = true;
	}
}

//run the shell
//prompt, read command and process in a loop
void runShell(){

	//user command
	char command[MAX_LENGTH_CMD_LINE];

	//program name, arguments, input/output redirection, background (&)
	char program[MAX_LENGTH_CMD_LINE];
	char arguments[MAX_ARGUMENTS][MAX_LENGTH_CMD_LINE];
	char inputRedirect[MAX_LENGTH_CMD_LINE];
	char outputRedirect[MAX_LENGTH_CMD_LINE];
	char background; //&
	int status = 0; //status
	
	int running = true; //running shell
	int numArguments; //number of arguments	

	//loop to receive command and process
	while(running == true){

		//check if background processes have been terminated
		checkBackgroundProcessTermination();

		printf(": ");
		fflush(stdout);

		//read a command
		bzero(command, MAX_LENGTH_CMD_LINE);//zero a byte string
		fgets(command, MAX_LENGTH_CMD_LINE, stdin);
		command[strlen(command) - 1] = '\0'; //remove new line

		//parse command
		parseCommand(command, program, arguments, inputRedirect,  outputRedirect, &background, &numArguments);

		//process command
		//printf("DEBUG: process command\n");
		running = processCommand(program, arguments, inputRedirect,  outputRedirect, background, numArguments, &status);

		//printf("DEBUG: Done one command\n");
	}//end while

	
}

//main function to start c application
int main(){

	//PIDs of noncompleted background processes in an array
	PIDs = (pid_t*)malloc(BACKGROUND_PROCESSES_DEFAULT_SIZE * sizeof(pid_t));

	//register SIGINT signal
	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = catchSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = SA_RESETHAND | SA_NODEFER;

	//register SIGINT (^C)
	sigaction(SIGINT, &SIGINT_action, NULL);

	//register SIGTSTP signal
	struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = catchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;

	//register SIGTSTP (^Z)
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	//run shell
	runShell();

	//free resource
	free(PIDs);

	return 0;
}
