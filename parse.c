#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parse.h"


//initialize CommandLine struct
void initCommandLine(CommandLine *cLine) {
	cLine->size = 0;
	cLine->isIn = 0;
	strcpy(cLine->inFile, "");
	cLine->isOut = 0;
	strcpy(cLine->outFile, "");
	cLine->isAppend = 0;
	strcpy(cLine->variable, "");
	strcpy(cLine->value, "");
	cLine->setVariable = 0;
} 


//print contents of CommandLine struct in the proper format
void printCommandLine(CommandLine *cLine) {
	printf("%d:", cLine->size);
	if(cLine->isIn) //print input file
		printf(" <'%s'", cLine->inFile);
	int i;
	int j;
	for(i = 0; i<cLine->size; ++i) { //print each command seperated by a '|'
               	for(j = 0; j<cLine->commandSize[i]; j++) 
                        printf(" '%s'", cLine->commands[i][j]);
		if((i+1) != cLine->size)
			printf(" |");	
	}
	if(cLine->isAppend) //print output file
		printf(" >>'%s'", cLine->outFile); 
	else if(cLine->isOut)
		printf(" >'%s'", cLine->outFile);
	printf("\n");
	
}

int checkVariable(char temp[100], int size) {//checks whether a string is a proper shell variable
	if(size == 0 || (size == 1 && temp[0] == '_'))
		return 0;
	int i;
	for(i = 0; i<size; i++) {
		char c = temp[i];
		if(c != '_' && !isdigit(c) && !isalpha(c))
			return 0;
	}
	return 1;
}
void Parse(CommandLine *cLine, char *line) {
	int i;
	char tmpLine[10000];
	strcpy(tmpLine, line);
	int cCount = 0;
	char currentCommand[100][100];
	char c = tmpLine[0];
	char last = '\0';
	int strLen = strlen(tmpLine);
	int toSet = 0;
	for(i = 0; i<strLen; ++i) {   //go through command line character by character
		c = tmpLine[i];
		if (c == ' ')
			continue;
		else if(c == '|') { //add currentCommand as a command to commands array
			int j;
			for(j = 0; j<cCount; j++)
				strcpy(cLine->commands[cLine->size][j], currentCommand[j]);
			cLine->commandSize[cLine->size++] = cCount;
			cCount = 0;	//reset currentCommand
		}
		else if(c == '<') { //set isIn to 1, last to 'i'
			last = 'i';
			cLine->isIn = 1;
		} 
		else if(c == '>') {
			if((i+1) != strLen && tmpLine[i+1] == '>') { //if next character is '>' set last to 'a', isAppend to 1, isOut to 1, move to next character
				++i;
				last = 'a';
				cLine->isAppend = 1;
				cLine->isOut = 0;
			}
			else { //else set last to 'o', isOut to 1, isAppend to 1 
				last = 'o';
				cLine->isOut = 1;
				cLine->isAppend = 0;
			}
		}
		else {
			int quoted = 0;
			char temp[100];
			int size = 0;
			if(c == '\'') { //implementing single quotes, while c is not ' append to temp;
				quoted = 1;
				for(c = tmpLine[++i]; c != '\'' && c != '\0'; c = tmpLine[++i]) {//add characters to temp until EOF or ' character
					temp[size++] = c;
				}
				if(c == '\0') {//if user presses enter before finishing quote, get the rest of the quote off stdin
					char input[1000];
					int quoteFin = 0;
					while(!quoteFin) {
						printf("\\?"); //show prompt that bash shows
						fgets(input, 1000, stdin);
						int tmpi;
						size_t tmpLength = strlen(input);
						input[tmpLength-1] = '\0';
						tmpLength = strlen(input);
						if(strcmp(input, "") != 0){ 
							temp[size++] = ' ';
							char tmpString[1000];
							int indexOftmpString = 0;
							int replace = 0;
							for (tmpi = 0; tmpi<tmpLength; ++tmpi){
								c = input[tmpi];	
								if(quoteFin){
									tmpString[indexOftmpString++] = c;
									replace = 1;
								}
								else if(c =='\'')
									quoteFin = 1;
								else	
									temp[size++] = c;
							
							}
							if(replace){
								tmpString[indexOftmpString] = '\0';	
								i = 0;
								strLen = indexOftmpString;
								strcpy(tmpLine, tmpString);												
							}
						}
					}
				}	
			}
			else {
				while(!strchr(" <>|'", c)) {//while c is not a special character, space, or null append it to string temp
					if(cLine->size == 0 && c == '=' && checkVariable(temp, size)) {//if the current character is '=' and the previous string can be a shell variable
						toSet = 1;					       //set toSet to 1
						break;
					}
					else if(c == '$') {
						char replacetp[100];
						int blah = 0;
						c = tmpLine[++i];
						while(!strchr(" <>|'$", c)) {
							replacetp[blah++] = c;
							c = tmpLine[++i];
						}
						replacetp[blah] = '\0';
						char *tp = getenv(replacetp);
						if(tp) {
							strcat(temp, tp);
							size += strlen(tp); 	
						}
					}
					else {
						temp[size++] = c;	
						c = tmpLine[++i];
					}
				}
				if(strchr("<>|'", c))//if c ended at a special charcter subract i by 1
					--i;
			}
			temp[size] = '\0';
			if(toSet) { //for implementing environment variables
				if(cLine->setVariable) {//if cLine already set cLine-> variable set cLine->value to temp
					toSet = 0;
					strcpy(cLine->value, temp);
				}
				else {
					cLine->setVariable = 1;
					strcpy(cLine->variable, temp);
				}	
			}
			else if(last) { //if last is not null
				if(last == 'a') //if last is 'a' copy temp to string appendFile
					strcpy(cLine->outFile, temp);		
				else if(last == 'o') //if 'o', copy to string outFile
                                        strcpy(cLine->outFile, temp);
				else //if 'i', copy to string inFile
					strcpy(cLine->inFile, temp);
				last = '\0';
			}
			else{	//insert string to current command
				if(!quoted) {
					char *token = strtok(temp, " ");
					while (token != NULL) {
						strcpy(currentCommand[cCount++], token);
						token = strtok(NULL, " ");
					}
				}
				else			
					strcpy(currentCommand[cCount++], temp);
			}
			strcpy(temp, "");//reset temp
	
		}

	}
	if(cCount != 0) {//after forloop, add string currentCommand to string array commands 
        	int j;
                for(j = 0; j<cCount; j++)
                        strcpy(cLine->commands[cLine->size][j], currentCommand[j]);
		cLine->commandSize[cLine->size++] = cCount;
	}
}

