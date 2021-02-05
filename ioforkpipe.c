#ifndef EXEC_C
#define EXEC_C
#include "pathsearchfull.c"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>

int shell_exit(pidlist * pids, time_t start, time_t longest_ps);
void double_pipe(tokenlist * input_tokens, pidlist *pids, time_t start,time_t longest_ps);
void sleep_exec(tokenlist * input_tokens, pidlist * pids, int pipe);
void check_pids(pidlist * pids);
void jobs(pidlist * pids);
void triple_pipe(tokenlist * input_tokens, pidlist *pids, time_t start, time_t longest_ps);
void copy_tokenlist(tokenlist* dest, tokenlist * src, int start, int end);
char * cm_path_cat(char * path, char * command);
int exec_tokenlist(char ** cmd_path,tokenlist * tokens, pidlist * pids, time_t start, time_t longest_ps);
int input_has_symbol(tokenlist * input, const char * symbol);

//Input Output redirection
//----------------------------
void inputOut(char *left, char *right, int flag, char *path, pidlist * pids, int background);
void parseIO(char *input, int flag, char *path, pidlist * pids, int background);
void externalCommand2(char *input, char *path);
int isInputOut(char *token);

int isInputOut(char *token)
{
	char *left = NULL;
	char *right = NULL;
	int output = 0;
	int input = 0;
	int flag = 0;
	
	while(*token)
	{
			if (strchr(">", *token))
			output = 1;
			token++;
	}
	while(*token)
	{
		if(strchr("<", *token))
		input = 1;
		token++;
	}	
	if(input == 1)
		return 2;
	if(output == 1)
		return 1;
	else return 0;
	
}
void parseIO(char *input, int flag, char *path, pidlist * pids, int background)
{
	//if flag = 1 then we have output >
	//if flag = 2 then we have input <
	tokenlist *tokens = new_tokenlist();
	if(flag == 2)
	{
		//resetting the path to get the correct one
		tokens = get_tokens_d(input, '<');
		char *path = NULL;
		path = get_abs_path(tokens-> items[1]);
	}
	else if(flag == 1)
	tokens = get_tokens_d(input, '>');
	
	char *right = strtok(tokens-> items[1], " ");
			//left				//right	
	inputOut(tokens-> items[0], right, flag, path, pids, background);
	free_tokens(tokens);

}	

void inputOut(char *left, char *right, int flag, char *path, pidlist * pids, int background )
{
	//if flag = 1 then we have output >
	//if flag = 2 then we have input <
	pid_t pid = fork();
		
	if(flag == 1)
	{
		if(pid == 0)
		{
			int fd = creat(right, 0666);
			//int fd = open(right, O_RDWR | O_CREAT, S_IRWXU);
			//printf("File descriptor is: %d", fd);
			//printf("'\n'");
			
			//error case
			if(fd < 0)
			{
				printf("Error in redirecting output to: %s", right);
				exit(0);
			}
			else
			{
				//close(1); 	//closing stdout
				dup2(fd, 1);	//duplicate the file descriptor
				close(fd);		//closing original file descriptor
				externalCommand2(left, path);
				free(path);
				
			}
		}
		else if(pid != 0)
		{
			if(background != -1)
			{
				tokenlist * copy = new_tokenlist();
				copy = get_tokens_d(left, ' ');
				add_pid(pids, pid, copy);
				free(copy);
				waitpid(pid, NULL, WNOHANG);
			}else	{
				waitpid(pid, NULL, 0);
			}
		}
	}
	
	else if(flag == 2)
	{	
		if(pid == 0)
		{
			//int fd = creat(left, 0666);
			int fd = open(left, O_RDWR | O_CREAT, S_IRWXU);
			
			if(fd < 0)
			{
				printf("Error in redirecting input to: %s", left);
				printf("\n");
				exit(0);
			}
			else
			{
				dup2(fd, 0);
				close(fd);
				externalCommand2(right, path);
				free(path);
			}
		}
		else if(pid != 0)
		{
			if(background != -1)
			{
				tokenlist * copy = new_tokenlist();
				copy = get_tokens_d(right, ' ');
				add_pid(pids,pid, copy);
				free(copy);
				waitpid(pid, NULL, WNOHANG);
			}else
			waitpid(pid, NULL, 0);
		}
	}
}

void externalCommand2(char *input, char *path)
{
	strcpy(input, path);
	
	char *x[2];
	x[0] = input;
	x[1] = NULL;
	//printf("x is %s", x[0]);
	//printf("\n");
	
	execv(x[0], x);
	//printf("Executed correctly. ");
	
}


/*
 * This function checks each input token to see if it 
 * represents the symbol param. If there is a pipe, returns the index
 * of the first pipe in the tokenlist. Otherwise, returns
 * -1 
 */
int input_has_symbol(tokenlist * input, const char * symbol)
{
	int pipe = -1;
	int i = 0;

	while(pipe == -1 && i < input ->size)
	{
		if(strcmp(input -> items[i], symbol) == 0)
			pipe = i;
		else
			i++;
	}

	return pipe;
}

/*This function assumes that no special operations are included 
 * in the command line. It executes the command and given parameters, 
 *depending on whether the given command is built in or an executable
* found somewhere in the PATH directory list. 
*/
int exec_tokenlist(char ** cmd_path,tokenlist * tokens, pidlist * pids, time_t start, time_t longest_ps)
{
		int exit_time = 0;
	if(cmd_is_builtin(tokens)) // if dealing w a builtin
	{
		char * cmd = tokens -> items[0]; // determine which one
		if(strcmp(cmd, "echo") == 0) 	 // and run accordingly
			printf("run echo\n");
		else if	(strcmp(cmd, "exit") == 0)
			exit_time = shell_exit(pids, start, longest_ps);
		else if	(strcmp(cmd, "jobs") == 0)
			jobs(pids);	
		else // must be cd
			cd(tokens);
	 }else{ 			// if not builtin, fork and exec
		int pid = fork();
		if(pid == 0)
		{
			if(cmd_has_slash(tokens)) //cmd has path included
				execv(tokens -> items[0], tokens -> items);
			else 	// must determine path before exe
			{
				*cmd_path = get_abs_path(tokens->items[0]);
				if(*cmd_path == NULL)	//if no path, report err
					printf("%s: Command not found\n",tokens -> items[0]);
				else // execute cmd with abs path of executable
					execv(*cmd_path, tokens -> items);
			}

		}else
		{	
			waitpid(0, NULL, 0);	
		}	
	}
		
		return exit_time;
}
/* This function checks all the running processes in the shell's
 * process stack and if any are done, the finished process'
 * information is printed out and it is removed from the stack. 
 */
void check_pids(pidlist * pids)
{
	int pid = 0;
	for(int i = 0; i < pids -> size; i++)	// iterate through stack
	{
		pid = pids -> items[i] -> number; // get pid number from ps
		if(waitpid(pid, NULL, WNOHANG) !=(pid_t)0)//if ps done
		{
			printf("[%d]+ ",i + 1); //print out info
			print_tokenlist_full(pids ->items[i] -> cmdline);
			remove_pid(pids, i); // remove ps from stack
		}
		//else keep checking other processes in stack
	}
}	

/*This function goes through the list of pids and if the process 
 * is still running, it prints the relevant information for the process.
 */
void jobs(pidlist * pids)
{
	for(int i = 0; i < pids -> size; i++) // for each pid
	{
		int pid = pids ->items[i] -> number;
		if(waitpid(pid, NULL, WNOHANG) ==0) // if still running
		{
			printf("[%d]+ %d ", i+1, pid); // print info
			print_tokenlist_full(pids -> items[i] -> cmdline);
		}
	}
}
/*This function assumes that the given input contains a &. It makes
 * a copy of the input tokens without the & and runs the command. If
 * this function isn't being called within a pipe (i.e., the pid
 * hasn't already been added in the pipe function), then the 
 * pid is added to the pidlist. Once this is done, the function returns
 * without waiting for the process to complete.
 */
void sleep_exec(tokenlist * input_tokens, pidlist * pids, int pipe)
{

	char * cmd_path = NULL;	

	int pid = fork();

	tokenlist * copy = new_tokenlist(); // copy input without & 
	copy_tokenlist(copy, input_tokens, 0, input_tokens -> size - 1);

	if(pid == 0) // exec child
	{		
		cmd_path = get_abs_path(copy -> items[0]);
		execv(cmd_path, copy ->items);
	}else	{

		waitpid(pid, NULL, WNOHANG); // don't wait (not really necessary)

		if(pipe == 0) //if not exec'd from pipe func, add pid
			add_pid(pids, pid, input_tokens); 

		if(cmd_path!= NULL) // clean up char * and tokenlists
			free(cmd_path);	
		if(copy != NULL)
			free(copy);
	}
	
	return;
}
/*This function assumes the input contains only one | symbol.
 * It splits the entire input into two separate cmds and 
 * pipes the left hand side (CMD 1) into the right hand side
 * (CMD2) and then prints the results to stdout. If CMD2 contains
 * a & symbol, then CMD2 is executed in the background.
 */
void double_pipe(tokenlist * input_tokens, pidlist *pids, time_t start, time_t longest_ps)
{
	
	int p_fds[2]; // for pipe file descriptors
	char * cmd_path = NULL;	
	int pipe_index = input_has_symbol(input_tokens, "|"); //get index of pipe

	tokenlist * cmd1 = new_tokenlist(); // make tokenlists for CMD 1 & 2
	tokenlist * cmd2 = new_tokenlist();
	copy_tokenlist(cmd1, input_tokens, 0, pipe_index); // copy appropriately 
	copy_tokenlist(cmd2, input_tokens, pipe_index +1, input_tokens -> size);
	
	pipe(p_fds); // apply pipe file descriptors
	int pid1 = fork(); // fork for first process 

	if(pid1 ==0)  // redirect output for CMD1
	{

		close(p_fds[0]); // close unused end of pipe(read end)
		close(1); // close stdout 
		dup(p_fds[1]); // dup write end of pipe; now it replaces stdout

		exec_tokenlist(&cmd_path, cmd1, pids, start, longest_ps); // exec CMD1
		exit(1);
	}

	int pid2 = fork(); // fork for second process
	

	if( pid2 == 0)
	{

		close(p_fds[1]); // close write end of pipe (unused)
		close(0); // close stdin
		dup(p_fds[0]); //dup read end of pipe; now it replaces stdin

		if(input_has_symbol(cmd2, "&") != -1) // if cmd2 is a bckgr ps
			sleep_exec(cmd2, pids, 1);	
		else 					// exec normally
			exec_tokenlist(&cmd_path, cmd2, pids, start, longest_ps);
		exit(1);

	}else if (input_has_symbol(cmd2, "&")){ // if CMD2 is bckgr ps, add
		add_pid(pids, pid2, input_tokens); // pid with cmdline CMD1 |CMD 2&
	}
	
	close(p_fds[0]);
	close(p_fds[1]); // close off both ends of pipe

	waitpid(pid1, NULL, 0);
	if( input_has_symbol(cmd2, "&")) //if cmd2 is bckgrps, don't wait
		waitpid(pid2, NULL, WNOHANG); 
	else				// else wait
		waitpid(pid2, NULL, 0); 
	
	if(cmd_path != NULL) // clean up char * and tokens
		free(cmd_path);
	free_tokens(cmd1);
	free_tokens(cmd2);	
}
/*Similar to the double pipe function execpt that 
 * it doesn't support bckgr processing in any of the 
 * commands. Runs CMD1 with input from stdin, pipes 
 * output as input for CMD2,  pipes output from CMD2
 * as input for CMD3, and prints to stdout. The 
 * one key difference is the extra step of parsing
 * CMD 1 from CMD 1 | CMD 2 | CM3 and then parsing
 * CMD 2 and CMD 3 from CMD2 | CMD 3.
 */
void triple_pipe(tokenlist * input_tokens, pidlist *pids,time_t start, time_t longest_ps)
{
	
	int p1_fds[2];	
	int p2_fds[2];
	char * cmd_path = NULL;	
	int pipe_one_index = input_has_symbol(input_tokens, "|");

	tokenlist * cmd1 = new_tokenlist(); // copy tokenlists for CMD1, 2, 3
	tokenlist * cmd2 = new_tokenlist();
	tokenlist * cmd3 = new_tokenlist();
	tokenlist * test = new_tokenlist();
	// parse CMD1 from CMD1 | CMD 2 | CMD 3	
	copy_tokenlist(cmd1, input_tokens, 0, pipe_one_index);

	//parse CMD 2 | CMD 3 from CMD 1 | CMD 2 | CMD 3
	copy_tokenlist(test, input_tokens, pipe_one_index +1, input_tokens -> size);

	//get pipe index in CMD 2 | CMD 3
	int pipe_two_index = input_has_symbol(test, "|"); 

	//parse CMD 2 from CMD 2 | CMD 3
	copy_tokenlist(cmd2, test, 0, pipe_two_index);

	// parse CMD 3 from CMD2 | CMD 3
	copy_tokenlist(cmd3, test, pipe_two_index +1, test -> size);
	
	pipe(p1_fds);
	pipe(p2_fds);
	int pid1 = fork(); // fork for first process 

	if(pid1 ==0)  // redirect output for ps1 
	{

		close(p1_fds[0]); // close unused end of pipe(read end)
		close(1); // close stdout 
		dup(p1_fds[1]); // dup write end of pipe; now it replaces stdout

		exec_tokenlist(&cmd_path, cmd1, pids, start, longest_ps); // exec CMD 1
		exit(1);
	}

	int pid2 = fork(); // fork for second process
	if( pid2 == 0)
	{
		close(p1_fds[1]); // unused write end of pipe one
		close(p2_fds[0]); // unused read end of pipe two
		close(0); // close stdin
		dup(p1_fds[0]); //dup read end of pipe; now it replaces stdin
		close(1); // close stdout
		dup(p2_fds[1]); // dup write end of pipe two

		exec_tokenlist(&cmd_path, cmd2, pids, start, longest_ps); // exec CMD 2
		exit(1);
	} 

	int pid3 = fork(); // fork for third process
	if( pid3 == 0)
	{
		close(p1_fds[0]); // close  both ends of pipe one
		close(p1_fds[1]);
		close(p2_fds[1]); //close write end of pipe two
		close(0); // close stdin
		dup(p2_fds[0]); //dup read end of pipe two; now it replaces stdin

		exec_tokenlist(&cmd_path, cmd3, pids, start, longest_ps); // exec CMD 3
		exit(1);
	} 
	
	close(p1_fds[0]);
	close(p1_fds[1]); // close off both ends of pipe one
	close(p2_fds[0]);
	close(p2_fds[1]); // close off both ends of pipe two

	waitpid(pid1, NULL, 0); // wait for all processes before returning
	waitpid(pid2, NULL, 0);
	waitpid(pid3, NULL, 0);
	
	if(cmd_path != NULL) // clean up all char * and tokenlist*
		free(cmd_path);
	free_tokens(cmd1);
	free_tokens(cmd2);	
	free_tokens(cmd3);
	free_tokens(test);

	return;
}
/*This is an intial function that assumes the input contains at least one |
 * symbol. It determines how many | symbols and depending on the amount,
 * it calls double_pipe for CMD 1 | CMD 2 or triple_pipe for CMD 1|CMD2|CMD3
 */
void pipe_cmd(tokenlist * input_tokens, pidlist * pids, time_t start, time_t longest_ps)
{

	int pipe_index = input_has_symbol(input_tokens, "|"); 
	
	// make copy of input to check input after first | for another |		
	tokenlist * test = new_tokenlist();
	copy_tokenlist(test, input_tokens, pipe_index +1, input_tokens -> size);

	if(input_has_symbol(test, "|") == -1) //if only one |
		double_pipe(input_tokens, pids, start, longest_ps);
	else // must be two |
		triple_pipe(input_tokens,pids, start, longest_ps);

	free_tokens(test); // clean up tokenlist 
	return;
}
/*This function checks to see if all background processes are done.
*If so, then print the longest running process and the total time 
* the shell ran, then return the flag to the main() loop to indicate
* that it is now time to terminate the program.
*/
int shell_exit(pidlist * pids, time_t start, time_t longest_ps)
{
	printf("Waiting for all processes to terminate...\n");
	while(pids -> size != 0)
	{
		check_pids(pids);
	}
	time_t shell_time = time(NULL) - start;
	printf("Longest process ran for %d sec\n", longest_ps);
	printf("Shell ran for %d sec\n", shell_time);
	return 1;
}


int main()
{
	time_t start = time(NULL);
	time_t longest_ps = 0;
	pidlist * pids = new_pidlist();
	int exit = 0;
	while(exit != 1)
	{	
		
		printf(">");
		char * cmd_path = NULL;
		char * input_str = get_input();
		tokenlist * input_tokens = get_tokens_d(input_str, ' ');
		int flag = -1;
		int io = 0;
		time_t ps_time = time(NULL);
		for(int i=0; i < input_tokens-> size; i++)
		{
			flag = isInputOut(input_tokens-> items[i]);
			if(flag == 1)
			{
				int background = input_has_symbol(input_tokens, "&");
				io = 1;
				//1 means output > so we need path of
				//left side of >
				char *path = get_abs_path(input_tokens-> items[0]);
				parseIO(input_str, flag, path, pids, background);
			}	
			else if(flag == 2)
			{
				int background = input_has_symbol(input_tokens, "&");
				io = 1;
				//2 means input < so we need path of
				//right side of <
				char *path = get_abs_path(input_tokens-> items[2]);
				parseIO(input_str, flag, path, pids, background);
			}
		}
		
		if(io == 0)
		{
			if(input_has_symbol(input_tokens, "|") != -1)
				pipe_cmd(input_tokens, pids, start, longest_ps);
			else if(input_has_symbol(input_tokens, "&") != -1)
					sleep_exec(input_tokens, pids, 0);
			else
			exit = exec_tokenlist(&cmd_path,input_tokens, pids,start, longest_ps);
		}
		ps_time = time(NULL) - ps_time;
		if(ps_time > longest_ps)
			longest_ps = ps_time;
		
		if(strcmp(input_tokens -> items[0], "jobs") != 0)
			check_pids(pids);
		
		if(input_str != NULL)
			free(input_str);
		if(cmd_path != NULL)
			free(cmd_path);
		free_tokens(input_tokens);
	}
	free(pids);
	return 0;
}
#endif
