#ifndef EXEC_C
#define EXEC_C
#include "pathsearchfull.c"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>


void copy_tokenlist(tokenlist* dest, tokenlist * src, int start, int end);
char * cm_path_cat(char * path, char * command);
void exec_tokenlist(char ** cmd_path,tokenlist * tokens);
int input_has_symbol(tokenlist * input, const char * symbol);
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


int exec_command(tokenlist * input_tokens)
{
			
	
	int pid = fork();
	char * cmd_path = NULL;
	if(pid == 0) // child process
	{
		exec_tokenlist(&cmd_path,input_tokens);
	} else  // parent process
	{
		if(cmd_path != NULL)
			free(cmd_path);
		waitpid(0, NULL, 0);
		printf("Parent done\n");

	}

}

void exec_tokenlist(char ** cmd_path,tokenlist * tokens)
{
	int pid = fork();
	if(pid == 0)
	{
		
		if(cmd_is_builtin(tokens))
			printf("Is builtin function\n");
		else if(cmd_has_slash(tokens))
			printf("contains a slash");
		else 
		{
			*cmd_path = get_abs_path(tokens->items[0]);
			if(*cmd_path == NULL)	
				printf("%s: Command not found\n",tokens -> items[0]);
			else
				execv(*cmd_path, tokens -> items);
		}

	}else
	{	
		waitpid(0, NULL, 0);	
	}	
		
}

void check_pids(pidlist * pids)
{
	int pid = 0;

	for(int i = 0; i < pids -> size; i++)
	{
		pid = pids -> items[i] -> number;
		if(waitpid(pid, NULL, WNOHANG) != 0)
			printf("[%d] Done %s\n",i, pids -> items[i] -> name);
		else
			printf("[%d] Running %s\n",i, pids ->items[i] -> name);
	}
}	

void sleep_exec(tokenlist * input_tokens, pidlist * pids)
{
	char * cmd_path = NULL;	
	int pid = fork();

	tokenlist * copy = new_tokenlist();
	if(pid == 0)
	{	
		copy_tokenlist(copy, input_tokens, 0, input_tokens -> size - 1);
		cmd_path = get_abs_path(input_tokens -> items[0]);
		execv(cmd_path, copy -> items);
	}else	{
			
		add_pid(pids, pid, input_tokens-> items[0]);
		if(cmd_path!= NULL)
			free(cmd_path);	
		if(copy != NULL)
			free(copy);
	}
}
void pipe_cmd(tokenlist * input_tokens)
{
	int p_fds[2];
	char * cmd_path = NULL;	
	int pipe_index = input_has_symbol(input_tokens, "|");
	if(pipe_index != -1)
	{		
		
		tokenlist * ps1 = new_tokenlist();
		copy_tokenlist(ps1, input_tokens,0, pipe_index);
		
		tokenlist * ps2 = new_tokenlist();
		copy_tokenlist(ps2, input_tokens,pipe_index +1, input_tokens -> size);

		pipe(p_fds);
		int pid1 = fork(); // fork for first process 

		if(pid1 ==0)  // redirect output for ps1 
		{
			printf("executing ps1\n");
			cmd_path = get_abs_path(ps1->items[0]);
			printf("cmd_path: %s\n", cmd_path);
			close(p_fds[0]); // close unused end of pipe(read end)
			close(1); // close stdout 
			dup(p_fds[1]); // dup write end of pipe; now it replaces stdout
			int result = execv(cmd_path, ps1 -> items);
			printf("shouldn't see this\n");
			exit(1);
		}

		int pid2 = fork(); // fork for second process
		if( pid2 == 0)
		{
			cmd_path = get_abs_path(ps2->items[0]);

			close(p_fds[1]); // close write end of pipe (unused)
			close(0); // close stdin
			dup(p_fds[0]); //dup read end of pipe; now it replaces stdin
			printf("executing ps2\n");
			int result =execv(cmd_path, ps2 -> items);
			printf("shouldn't see this\n");
			exit(1);
		} 
		
		close(p_fds[0]);
		close(p_fds[1]); // close off both ends of pipe

		waitpid(pid1, NULL, 0);
		waitpid(pid2, NULL, 0);
		
		if(cmd_path != NULL)
			free(cmd_path);
		free(ps1);
		free(ps2);	
	}else	{
		printf("nopipe\n");
	}
	return;
}


int main()
{
	pidlist * pids = new_pidlist();
	while(1)
	{
		printf(">");
		char * input_str = get_input();
		check_pids(pids);
		tokenlist * input_tokens = get_tokens(input_str, ' ');
		char * cmd_path = NULL;
		sleep_exec(input_tokens, pids);
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
