#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

void changeDirectory(int size, char commands[100][100]) {
	/*changes the current directory to the specified directory.
	If none are specified,change to HOME directory */
	int changed;
	if(size == 1)
		changed = chdir(getenv("HOME"));
	else
		changed = chdir(commands[1]);
	if(changed < 0) 
		printf("Failed to change to directory. ERROR %d: %s\n",errno,strerror(errno));
              
}
void freeStrings(int size, char **stringArray) {//frees an array of srings from memory
	int i;
	for(i = 0; i<size; i++)
		free(stringArray[i]);
	free(stringArray);

}

void setOutFile(char *outFile, int isAppend) {//sets stdout to the outfile
	FILE *fp;
	if(isAppend)
		fp = fopen(outFile, "a");
	else
		fp = fopen(outFile, "w");
	dup2(fileno(fp), fileno(stdout));
	fclose(fp);
}


void runCommand(int commandSize,  char commandLine[100][100]) {
	/*Creates an input vector using commandLine and runs the command
 * 	using execvp.*/
	char command[100] = "";
	strcpy(command, commandLine[0]); //save name of command
	char **toExec = malloc(sizeof(char*) * 10000); //create input vector
	int current = 0;
	toExec[current] = malloc(sizeof(char*) * 100);
	strcpy(toExec[current++], command);
	int i;
	if(commandSize > 1) { 
		for(i = 1; i<commandSize; i++) {
			toExec[current] = malloc(sizeof(char*) * 100);
        		strcpy(toExec[current++], commandLine[i]);
		}
	}
	toExec[current] = malloc(sizeof(char*) * 100);
        toExec[current++] = NULL;
        if(execvp(command, toExec)) { //run command
		freeStrings(current, toExec);
		printf("'%s' failed with error %d: %s\n",command, errno,strerror(errno));
		exit(0);
	}
	freeStrings(current, toExec);
}

void pipeStuff(CommandLine *command) {
	//implement pipes
	int pipeNumber = command->size-1;
	int pipes[pipeNumber*2];
	int i;
	for(i = 0; i<pipeNumber; i++) //initialize pipes
		pipe(pipes + (i * 2)); 

	for(i = 0; i<command->size; i++) {
		if(fork() == 0) {//for each command, fork, set up pipes for the process, run the command
			if((i+1) < command->size) 
				dup2(pipes[i*2+1], fileno(stdout)); //redirect standard output if not last command
			if(i != 0) //redirect standard input if not first commmand 
				dup2(pipes[i*2-2], fileno(stdin));
			if((i+1) == command->size && (command->isOut || command->isAppend))
				setOutFile(command->outFile, command->isAppend);
			int j;
			for(j = 0; j<pipeNumber*2; j++)
				close(pipes[j]);
			runCommand(command->commandSize[i], command->commands[i]);
			exit(0); //just in case, but shouldn't be reachable
		}
	}
	int status;
	for (i = 0; i < pipeNumber*2; i++)
		close(pipes[i]);
	for (i = 0; i < command->size; i++)
    		wait(&status);

}

void processCommandLine(CommandLine *command){
	/* runs commands from CommandLine struct*/
	int in = dup(0); //save file descriptors for stdin and stdout
	int out = dup(1);
        if(strcmp(command->commands[0][0], "cd") == 0)//check if cd
                changeDirectory(command->commandSize[0], command->commands[0]);
        else {
		if(command->setVariable) {//check if a variable needs to be set
			setenv(command->variable, command->value, 1);
		}
		if(command->size != 0) {//if there are commands run the commands
			if(command->isIn){ //check for input redirection
				FILE *file = fopen(command->inFile, "r");
				if(file){
					dup2(fileno(file), fileno(stdin));
					fclose(file);
				}
				else{
					printf("Could not open file: \"%s\"\n", command->inFile);
					return;
				}
			}
        		pid_t pid = fork();
               		if(pid == -1)
                		perror("fork error");
                	else if (pid == 0) {
                        	if(command->size > 1) {//pipe if there is more than one command
                        		pipeStuff(command);
                               		exit(0);
                        	}
                        	else {
					if(command->isOut || command->isAppend) //check for output redirection
						setOutFile(command->outFile, command->isAppend);
                        		runCommand(command->commandSize[0], command->commands[0]);
                        	}	
               		}	
                	else {
                		int status;
                        	waitpid(pid, &status, 0);
                	}
		}
	}	
	dup2(in, 0); //reset stdin and stout
	dup2(out, 1);
	close(in);
	close(out);
}
int main(int argc, char **argv) {
	//main loop
	if(argc > 2){//should have a most one argument
		printf("nsh accepts at most one argument!\n");
		return 0;	
	}
        char *buff;
        size_t buffsize = 10000;
        buff = (char *)malloc(buffsize * sizeof(char));
        size_t i;
	if(argc == 2) { //if called with a script file
		FILE *file = fopen(argv[1], "r");
		if(file){//if file can be read, loop through file executing each line of commands
			while(getline(&buff, &buffsize, file) != -1) {
                		i = strlen(buff)-1;
                		buff[i] = '\0';
                		CommandLine command;
                		initCommandLine(&command);
                		Parse(&command,  buff);
                		//printCommandLine(&command); //print parsed command
                		processCommandLine(&command);
			}
		}
		else {
			printf("Could not open file: \"%s\"\n", argv[1]);
		}
		free(buff);
		return 0;
	}
	while(1) {	//else print prompt, and run fthe following command inputted by user
		printf("? ");
		getline(&buff,&buffsize,stdin);
		i = strlen(buff)-1;
		buff[i] = '\0';
        	if(feof(stdin)) { //if eof signal, break from loop
			printf("\n");
			free(buff);
			break;
		}
		CommandLine command;
        	initCommandLine(&command);
        	Parse(&command,  buff);
        	//printCommandLine(&command); //print parsed command
		processCommandLine(&command);
	}
	return 0;
}
