//P2-SSOO-22/23

// MSH main file
// Write your msh source code here

//#include "parser.h"

#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_COMMANDS 8


// files in case of redirection
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];


void siginthandler(int param)
{
	printf("****  Exiting MSH **** \n");
	//signal(SIGINT, siginthandler);
	exit(0);
}


/* Timer */
pthread_t timer_thread;
unsigned long  mytime = 0;

void* timer_run ( )
{
	while (1)
	{
		usleep(1000);
		mytime++;
	}
}

/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
	//reset first
	for(int j = 0; j < 8; j++)
		argv_execvp[j] = NULL;

	int i = 0;
	for ( i = 0; argvv[num_command][i] != NULL; i++)
		argv_execvp[i] = argvv[num_command][i];
}


/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
	/**** Do not delete this code.****/
	int end = 0; 
	int executed_cmd_lines = -1;
	char *cmd_line = NULL;
	char *cmd_lines[10];

	if (!isatty(STDIN_FILENO)) {
		cmd_line = (char*)malloc(100);
		while (scanf(" %[^\n]", cmd_line) != EOF){
			if(strlen(cmd_line) <= 0) return 0;
			cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
			strcpy(cmd_lines[end], cmd_line);
			end++;
			fflush (stdin);
			fflush(stdout);
		}
	}

	pthread_create(&timer_thread,NULL,timer_run, NULL);

	/*********************************/

	char ***argvv = NULL;
	int num_commands;
	// Initializing some variables that we will need in mycalc (add):
	const char *env_var = "Acc";
	const char *env_val = "0";
	setenv(env_var, env_val, 1);
	char result_str[2048]; // To use sprintf() in mycalc (add)


	while (1) 
	{
		int status = 0;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		// Prompt 
		write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

		// Get command
		//********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
		executed_cmd_lines++;
		if( end != 0 && executed_cmd_lines < end) {
			command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
		}
		else if( end != 0 && executed_cmd_lines == end) {
			return 0;
		}
		else {
			command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
		}
		//************************************************************************************************


		/************************ STUDENTS CODE ********************************/
		if (command_counter > 0) {
			if (command_counter > MAX_COMMANDS){
				printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
			}
			else {
				// Print command (uncomment below if you want to see it displayed )
				//print_command(argvv, filev, in_background);
				int P[2]; //array for the pipe
				int prev_input = 0; //this variable will contain at each iteration, the  file descriptor for the reading side of the previous pipe
				int i; //loop variable
				pid_t pid; //process identifier variable
				for(i = 0; i<command_counter; ++i) //iterate over all the commands in the line entered to the shell
				{	
					const char* command = argvv[i][0];
					const char* arg1 = argvv[i][1];
					const char* arg2 = argvv[i][2];
					const char* arg3 = argvv[i][3];
					
					// **MYCALC**
					if (strcmp(command, "mycalc") == 0)
					{
						if ((arg1 != NULL && arg2 != NULL && arg3 != NULL && argvv[i][4] == NULL))
						{ // Check that the command received precisely 3 arguments

							// Check that the type of the arguments is the expected one
							bool arg1_zero = strcmp(arg1, "0") == 0;
							bool arg3_zero = strcmp(arg3, "0") == 0;

							bool arg1_valid = atoi(arg1) != 0 || arg1_zero; // arg1 is valid if it is an integer
							bool arg2_valid = atoi(arg2) == 0; // arg2 is of expected type if it is a string
							bool arg3_valid = atoi(arg3) != 0 || arg3_zero; // arg3 is valid if it is an integer

							if (arg1_valid && arg2_valid && arg3_valid) 
							{
								int result, remainder, quotient;

								// Cast the arguments as integers so we can operate with them
								int operand1 = atoi(arg1);
								int operand2 = atoi(arg3);

								if (strcmp(arg2, "add") == 0)
								{
									result = operand1 + operand2;
									char *envAcc = getenv(env_var);  // Save Acc current value
									int acc_int = atoi(envAcc);  // Cast it as an integer
									int result_int = result + acc_int;  // The accumulated sum is the accumulated sum of the previous iteration plus the current result
									sprintf(result_str, "%d", result_int);  // Accumulated sum is casted as a string
									int envCode = setenv(env_var, result_str, 1);  // Change value of the env_var (Acc)
									if (envCode < 0)  // Handle errors when setting the environment variable
									{
										perror("Error setting environment variable");
									}
									fprintf(stderr, "[OK] %d + %d = %d; Acc %s\n", operand1, operand2, result, result_str); // Print success message in the standard error output
								}
								
								else if (strcmp(arg2, "mul") == 0)
								{
									result = operand1 * operand2;
									fprintf(stderr, "[OK] %d * %d = %d\n", operand1, operand2, result); // Print success message in the standard error output
										
									
								}

								else if (strcmp(arg2, "div") == 0) 
								{
									if (operand2 != 0)
									{
										quotient = operand1 / operand2;
										remainder = operand1 % operand2; 
										fprintf(stderr, "[OK] %d / %d = %d; Remainder %d\n", operand1, operand2, quotient, remainder); // Print success message in the standard error output
									}
									else
									{
										printf("[ERROR] Division by zero.\n"); // Print error message in the standard output
									}
								}

								else
								{																										   // Handle mathematical operator introduced is neather add, mul nor div
									printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n"); // Print error message in the standard output
								}
							}
							else
							{																										   // Handle recieved arguments of unexpected type
								printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n"); // Print error message in the standard output
							}
						}
						else
						{																										   // Handle incorrect number of arguments
							printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n"); // Print error message in the standard output
						}
					}
					// **MYCALC**
					else if (strcmp(command, "mytime") == 0) ///
					{
						if (arg1 == NULL) // Check the command has received 0 arguments
						{
							unsigned long HH = (mytime / (1000 * 60 * 60)) % 24; 
							unsigned long MM = (mytime / (1000 * 60)) % 60; 
							unsigned long SS = (mytime / 1000) % 60; 
							
							fprintf(stderr, "%02lu:%02lu:%02lu\n", HH, MM, SS); // Print the success message in the standard error output
						}
						else
						{
							fprintf(stdout, "[ERROR] The structure of the command is: mytime \n"); // Print error message in the standard output
						}
					}
					else
					//EXTERNAL COMMANDS
					{
						if (i != command_counter-1) //Pipe creation if the command is not the last one.
						{
							if (pipe(P) < 0) //pipe creation error handling
							{
								perror("Error creating the pipe\n");
								exit(-1);
							}
						}
						//Child process generation : In charge of executing the current command of the sequence
						pid = fork();
						if( pid == -1 ) //process generation error handling
						{
							perror("Error: fork() \n");
							exit(-1);
						}
						if (pid == 0)
						{
							//CHILD PROCESS (command process) : Distinguish between 3 cases -> beginning, middle or last command in the sequence.
							//About redirections :
							// on the first child check if input redirection must be done
							// on the last child check if the input redirection must be done
							// on any child check if the error redirection must be done
							if ( i != 0) //if not beginning command then redirect input with the previous pipe.
							{
								dup2(prev_input,STDIN_FILENO);
								close(prev_input); 
							}
							else
							{
								if (strcmp(filev[0],"0") != 0)
								{
									//input redirection:
									close(STDIN_FILENO);
									open(filev[0],O_RDONLY); //open the input file with reading permissions (since standard input is closed, the os will assign as file descriptor for the file in filev[0], the standard input descriptor ie : 0)
								}
							}

							if (i != command_counter -1) //if not the last one then redirect output with the created pipe
							{
								dup2(P[1], STDOUT_FILENO); 
								close(P[1]); //close writing side of the pipe
								close(P[0]);//close reading side of the pipe
							}
							else
							{
								if ( strcmp(filev[1], "0") != 0)
								{
									//output redirection:
									close(STDOUT_FILENO);
									open(filev[1], O_CREAT | O_WRONLY, 0666); //same as for the input case but with writing permission and create permission in the case the file doesnt exist already.
								}
							}
							if (strcmp(filev[2], "0") != 0 )
							{
								//error redirection:
								close(STDERR_FILENO);
								open(filev[2], O_CREAT | O_WRONLY, 0666); //same as for output redirection 
							}
							
							//Now that redirection step has been completed, child executes the command before terminating.
							execvp(argvv[i][0], argvv[i]); //if succesful, the call of execvp will replace the process image and next code wont be executed
							
							//otherwise, for some reason like maybe a wrong spelled command, the child process will read this line and we will exit it with -1 status value sent to the parent
							perror("Executing command error");
							exit(-1);

						}
						else
						{
							//PARENT PROCESS (shell process)

							//Only case to distinguish is whether or not the command has beeen the last one

							if ( i != command_counter-1) //if the command is not last , then update the file descriptor of the previous pipe to be the one for the one created last.
							{
								prev_input = P[0]; //so that for the next command, the child will inherit this variable and will be able to redirect its standard input to the inside of the previous pipe.
								close(P[1]);
							}
							else
							{
								//the command is the last one, so no pipe was created. The only open file is the input one for the previous pipe.
								close(P[0]); // close reading file descriptor for last pipe used (if only one command was in the sequence, this will return -1 because no pipe was ever created, but it wont affect the program)
							}
							
							// background or foreground cases:
							if ( in_background == 1)  //shell continues execution 
							{ 
								//print on screen the childs id that is in charge of executing the command sequence in the background
								printf("[%d]\n", pid); 
							}
							else
							{ 
								//shell waits for last child to finish.
								int waitCode = wait(&status);
								if (waitCode < 0)
								{
									perror("Error while waiting");
									exit(-1);
								}
							}
						}
					}
					
				}
			}
		}
	}
	
	return 0;
}

