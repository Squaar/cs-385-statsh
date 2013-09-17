/*
*  Matt Dumford
*  mdumfo2@uic.edu
*
*  look into ltermcap -- shell library for tab completiont
*/ 
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

int main(int argc, char **argv, char** envp){
	printf("Matt Dumford - mdumfo2\n");
	
	char *inputHistory[100];
	struct rusage rusageHistory[100];
	int i = 0;

	while(1){
		char *in;
		size_t inSize = 100;
		int numChars;
		
		printf("\n> ");

		//get input
		numChars = getline(&in, &inSize, stdin);
	
		//remove newline character
		if(in[numChars-1] == '\n'){
			in[numChars-1] = '\0';
			numChars --;
		}		
	
		if(numChars == -1){
			printf("Error reading input");
			exit(-1);
		}
		else if(numChars > 0){ //if the getline worked and the user actually gave input

			//check if user wants to quit
			if(!strcmp(in, "quit") || !strcmp(in, "exit") || !strcmp(in, "q")){
				int j;
				for(j=0; j<i; j++){
					free(inputHistory[j]);
				}
				exit(0);
			}
			else if(!strcmp(in, "stats")){ //print stats
				if(i == 0)
					printf("No stats to display yet!\n");
				else{
					int j;
					for(j=0; j<i; j++){
						printf("%s\n", inputHistory[j]);
						printf("\tUser time: %lu.%06lu (s)\n", rusageHistory[j].ru_utime.tv_sec, rusageHistory[j].ru_utime.tv_usec);
						printf("\tSystem time: %lu.%06lu (s)\n", rusageHistory[j].ru_stime.tv_sec, rusageHistory[j].ru_stime.tv_usec);
					}
				}
			}
			else{ //tokenize and run exec
				int numCommands = 0;
				char *commands[50];
				char *command = strtok(in, "|");

				while(command != NULL){
					commands[numCommands] = command;
					command = strtok(NULL, "|");
					numCommands++;
				}

				commands[numCommands] = NULL;
				
				int j;				
				for(j=0; j<numCommands; j++){
	
					//tokenize input
					int numToks = 0;
					char *input[50];
					char *tok = strtok(commands[j], " ");
					
					while(tok != NULL){
						input[numToks] = tok;
						tok = strtok(NULL, " ");
						numToks++;
					} 
					
					//end the array with NULL	
					input[numToks] = NULL;
					
					pid_t pid = fork();
	
					if(pid < 0){
						printf("Error forking!");
						exit(-1);
					}
					else if(pid == 0){ //child process
						execvp(*input, input);
		
						//if exec returns and this runs, something broke.
						printf("Command not found: %s\n", input[0]);
						exit(-1);
					}
					else{ //parent process
						struct rusage rusage;
						int status;
						
						pid_t pid2 = wait4(pid, &status, 0, &rusage);
						
						if(pid2 != -1 && WIFEXITED(status) && !WEXITSTATUS(status)){
							inputHistory[i] = (char *) malloc(strlen(commands[j]));
							strcpy(inputHistory[i], commands[j]);
							rusageHistory[i] = rusage;
							i++;
							
							printf("\tUser time: %lu.%06lu (s)\n", rusage.ru_utime.tv_sec, rusage.ru_utime.tv_usec);
							printf("\tSystem time: %lu.%06lu (s)\n\n", rusage.ru_stime.tv_sec, rusage.ru_stime.tv_usec);
						}
					}
				}
			}
		}
	}
}
