/*
*  Matt Dumford
*  mdumfo2@uic.edu
*
*  look into ltermcap -- shell library for tab completiont
*	pid = fork(); 
*	if (pid == 0) { 
*		fd = open("hello", O_RDONLY);
*		dup2(fd, STDIN_FILENO);
*		close(fd);
*		fd = open("world", O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU); 
*		dup2(fd, STDOUT_FILENO );
*		close(fd);
*/ 
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>

int main(int argc, char **argv, char** envp){
	printf("Matt Dumford - mdumfo2\n");
	
	int inputHistorySize = sizeof(char *) * 100;
	int rusageHistorySize = sizeof(struct rusage) * 100;
	char **inputHistory = malloc(inputHistorySize);
	struct rusage *rusageHistory = malloc(rusageHistorySize);
	int i = 0;

	while(1){
		char *in = NULL;
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
			printf("Error reading input\n");
			exit(-1);
		}
		else if(numChars > 0){ //if the getline worked and the user actually gave input

			//check if user wants to quit
			if(!strcmp(in, "quit") || !strcmp(in, "exit") || !strcmp(in, "q")){
				int j;
				for(j=0; j<i; j++){
					free(inputHistory[j]);
				}
				free(inputHistory);
				free(rusageHistory);
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
			else{ //tokenize commands
				int numCommands = 0;
				int commandsSize = sizeof(char *) * 50;
				char **commands = malloc(commandsSize);
				char *command = strtok(in, "|");

				while(command != NULL){
					if(numCommands == commandsSize/sizeof(char *)){
						commandsSize *= 2;
						commands =  realloc(commands, commandsSize); 
					}

					commands[numCommands] = command;
					command = strtok(NULL, "|");
					numCommands++;
				}

				commands[numCommands] = NULL;

				pid_t pids[numCommands];

				//set up array of pipes
				int pipes[numCommands+1][2];

				int j;				
				for(j=0; j<numCommands; j++){
					int numFiles = 0;
					char *files[10];
					char *file = strtok(commands[j], "<>");
					
					while(file !=NULL){
						files[numFiles] = file;
						file = strtok(NULL, "<>");
						numFiles++;
					}

 					if(numFiles == 2 && j == 0){ //first command, input through file.
						
					}
					else if(numFiles == 2 && j == numCommands-1){ // last command, output into file.
						files[1] = strtok(files[1], " "); // remove whitespace
						pipes[j+1][0] = open(files[1], O_WRONLY|O_CREAT|O_TRUNC, 00666);
					}
					else if(numFiles == 3 && numCommands ==1){ // only command input and output to/from files.
						
					}
					
					
					//tokenize input
					int numToks = 0;
					int inputSize = sizeof(char *) * 50;
					char **input = malloc(inputSize);
					char *tok = strtok(files[0], " ");
					
					while(tok != NULL){
						if(numToks == inputSize/sizeof(char *)){
							inputSize *= 2;
							input = realloc(input, inputSize);
						}

						input[numToks] = tok;
						tok = strtok(NULL, " ");
						numToks++;
					} 

					//end the array with NULL	
					input[numToks] = NULL;
					
					//create current pipe
					//if(j != numCommands-1)
						pipe(pipes[j]);

					//fork new process
					pids[j] = fork();
	
					if(pids[j] < 0){
						printf("Error forking!");
						exit(-1);
					}
					else if(pids[j] == 0){ //CHILD PROCESS 
						if(j != 0)
							dup2(pipes[j-1][0], 0);
						if(j != numCommands-1)
							dup2(pipes[j][1], 1);

						if(j == numCommands-1)
							close(pipes[j][0]);
						close(pipes[j-1][0]);
						close(pipes[j][0]);
						close(pipes[j][1]);

						//execute current instruction
						execvp(*input, input);
		
						//if exec returns and this runs, something broke.
						printf("Command not found: %s\n", input[0]);
						exit(-1);
					}
					else{ //PARENT PROCESS
						if(j != 0)
							close(pipes[j-1][0]);
						if(j == numCommands-1)
							close(pipes[j][0]);
						close(pipes[j][1]);
					}
					free(input);

				} //end commands loop

				//loop and wait for each child
				for(j=0; j<numCommands; j++){

					struct rusage rusage;
					int status;
					
					pid_t pid2 = wait4(pids[j], &status, 0, &rusage);	

					//if child process exited successfully
					if(pid2 != -1 && WIFEXITED(status) && !WEXITSTATUS(status)){
						if(i == inputHistorySize/sizeof(char *) - 1){
							inputHistorySize *= 2;
							inputHistory = realloc(inputHistory, inputHistorySize);
						}
						if(i == rusageHistorySize/sizeof(struct rusage) - 1){
							rusageHistorySize *= 2;
							rusageHistory = realloc(rusageHistory, rusageHistorySize);
						}

						printf("%s\n", commands[j]);

						inputHistory[i] = malloc(strlen(commands[j]) * sizeof(char));
						strcpy(inputHistory[i], commands[j]);
						rusageHistory[i] = rusage;
						i++;
							
						printf("\tUser time: %lu.%06lu (s)\n", rusage.ru_utime.tv_sec, rusage.ru_utime.tv_usec);
						printf("\tSystem time: %lu.%06lu (s)\n\n", rusage.ru_stime.tv_sec, rusage.ru_stime.tv_usec);
					}
				}

				free(commands);
			}
		}
		free(in);
	}
}
