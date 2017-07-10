#ifndef PARSE_H_   
#define PARSE_H_

typedef struct CommandLine {
        char commands[100][100][100];
	int commandSize[100];
        int size;
        int isIn;
        char inFile[100];
        int isOut;
        char outFile[100];
        int isAppend;
	char variable[100];
	char value[1000];
	int setVariable;
} CommandLine;

void initCommandLine(CommandLine *cLine);

void dinitCommandLine(CommandLine *cLine);

void printCommandLine(CommandLine *cLine);

void Parse(CommandLine *cLine, char *line);
#endif // PARSE_H_
