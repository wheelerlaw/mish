// Author: Wheeler Law
// Date: 4/14/14
// Description: MISH: MyInteractiveSHell. A simple shell that executes
// other commands on the system. 

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Define all the structures for the arguments. 

void verbose(int argc, char** argv);
void help(int argc, char** argv);
void history(int argc, char** argv);
void quit(int argc, char** argv);
int externalCommand(int argc, char** argv);
void historyAppend(int argc, char** command, char*** history);

int verboseFlag=0;

/** Main: The main function. Takes in the parameters and calls for input (will
 ** execute other programs if necessary) repeatedly until quit is called.
 ** @param int argc: the number of arguments passed to main.
 ** @param char** argc: the actualy arguments passed to main.
 ** @return int: the return value for main. 
 **/
int main(int argc, char** argv){
	/*
	if(argc!=1&&argc!=2){
		printf("Usage: mrEd <filename>\n");
		exit(EXIT_FAILURE);
	}*/
		
	char** command=0;
	char* buff=0;
	size_t len=0;
	size_t read;
	
	// Begin printing the shell and inputting the commands. 
	int count=1;
	printf("mish[%d]> ",count);
	
	char** historyTable=malloc(40);
	for(int i=0;i<10;i++){
		historyTable[i]=NULL;
	}
	while((read=getline(&buff,&len,stdin))!=-1){
		char* ptr;
		if((ptr=strchr(buff,'\n'))){
			*ptr='\0'; // Remove the newline character from the command. 
			read--;
		}
		
		char* tok=0;
		tok=strtok(buff," ");
		int i=0;
		
		int argc2=0;
		while(tok){ // Tokenize the command. 
			// "(i+2)" is for the extra 4 bytes needed for the NULL pointer 
			// needed for execvp. Otherwise it would be (i+1).
			command=realloc(command,4*(i+2));
			command[i]=malloc(strlen(tok)+1);
			strcpy(command[i],tok);
			tok=strtok(NULL," ");
			i++; argc2++; // Argc2: the argc to pass to the command. 
		}
		if(read){
			
			historyAppend(argc2,command,&historyTable);
			count++; // Increment the command number. 
			
			if(!strcmp("verbose",command[0])){
				verbose(argc2,command);
			}else if(!strcmp("help",command[0])){
				help(argc2,command);
			}else if(!strcmp("history",command[0])){
				command=realloc(command,12);
				command[1]=(char*)historyTable;
				command[2]=(char*)count;
				history(argc2,command);
			}else if(!strcmp("quit",command[0])){
				command[1]=buff;
				quit(argc2,command);
			}else{
				int returnValue=externalCommand(argc2,command);
				if(returnValue){
					printf("command status: %d\n",returnValue);
				}
			}
		}
		
		// Free the command array. 
		for(int j=0;j<i;j++){
			free(command[j]);
		}
		free(command);
		command=0;
		
		// Iterate the command number and print out the next shell line. 
		
		printf("mish[%d]> ",count);
	}
	
	if(read==-1){
		printf("\n");
	}
	
	free(buff);
	free(command);
}

/** verbose: Toggles the verbosity of the output.
 ** @param int argc: the number of arguments in the arguments array. 
 ** @param char** argv: the argument array itself. Could contain things that
 **						are not strings that need to be casted.
 **/
void verbose(int argc, char** argv){
	if(argc!=2){
		printf("usage: verbose [on|off]\n");
		return;
	}
	if(!strcmp("on",argv[1])){
		verboseFlag=1;
	}else if(!strcmp("off",argv[1])){
		verboseFlag=0;
	}
}

/** help: Prints out a list of internal commands for mish.
 ** @param int argc: not used.
 ** @param char** argv: not used.
 **/
void help(int argc, char** argv){
	printf("Commands available for use:\n");
	printf("verbose [on|of]: turns on verbose output on or off for the shell\n");
	printf("help:            if you are looking at this entry now, you\
 probably don't\n                 need help finding this command\n");
	printf("history:         prints out a list of the last 10 commands that\
 were entered\n");
	printf("quit:            quits out of mish\n.");
}

/** history: Prints out the last 10 commands that were run, if any.
 ** @param int argc: the number of arguments in the arguments array. 
 ** @param char** argv: the argument array itself. Could contain things that
 **						are not strings that need to be casted.
 **/
void history(int argc, char** argv){
	char** historyTable=(char**)argv[1];
	int count=(int)argv[2];
	
	for(int i=0;i<10;i++){
		if(historyTable[i]!=NULL){
			printf("%d: %s\n",count-i-1,historyTable[i]);
		}
	}
}

/** quit: Quits out of mish. 
 ** @param int argc: not used.
 ** @param char** argv: argv[1] is the pointer to the input buffer that needs
 ** 					to be freed.
 **/
void quit(int argc, char** argv){
	for(int j=0;j<argc+1;j++){
			free(argv[j]);
	}
	free(argv);
	argv=0;
	
	exit(EXIT_SUCCESS);
}

void historyAppend(int argc, char** command, char*** history){
	for(int i=9;i>=0;i--){
		(*history)[i+1]=(*history)[i];
	}
	
	int totalLength=0;
	for(int i=0;i<argc;i++){
		totalLength+=strlen(command[i]);
	}
	(*history)[0]=malloc(totalLength+argc);
	
	int prevStringLength=0;
	for(int i=0;i<argc;i++){
		strcpy((*history)[0]+prevStringLength,command[i]);
		(*history)[0][strlen(command[i])]=' ';
	}
}

/** Executes any external commands not found on the internal commands list. 
 ** @param int argc: the number of arguments in the arguments array. 
 ** @param char** argv: the arguments array. 
 ** @return int status: returns the return value of the forked command. 
 **/
int externalCommand(int argc, char** argv){
	pid_t childPid,myPid;
	int status;
	
	myPid=getpid();
	childPid=fork();
	switch(childPid){
	case -1:
		perror("fork");
		exit(EXIT_FAILURE);
	case 0:
		myPid=getpid();
		
		if(verboseFlag){
			printf("        command: ");
			for(int i=0;i<argc;i++){
				printf("%s ",argv[i]);
			}
			printf("\n\n        input command tokens:\n");
			for(int i=0;i<argc;i++){
				printf("        %d: %s\n",i,argv[i]);
			}
			printf("        wait for pid %d: %s\n",myPid,argv[0]);
			printf("        execvp: %s\n",argv[0]);
		}
	
		argv[argc]=NULL;
		execvp(argv[0],argv);
		perror(argv[0]);
		_exit(EXIT_FAILURE);
		break;
	default:
		break;
	}
	
	myPid++;
	childPid=wait(&status);
	if(childPid<0){
		perror("wait");
	}
	
	if(WIFEXITED(status)){
		return(WEXITSTATUS(status));
	}else if(WIFSIGNALED(status)){
		return(WTERMSIG(status));
	}else{
		return 0;
	}
}



