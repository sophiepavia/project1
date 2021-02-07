#define _POSIX_C_SOURCE  200809L // necessary for setenv
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h> // for PATH searching

typedef struct {
	int size;
	char **items;
} tokenlist;

typedef struct
{
	tokenlist * cmdline;
	int number;	
	int timer;
} pid;

typedef struct {
	int size;
	pid *items[10];
} pidlist;

int shell_exit(pidlist * pids, time_t start, time_t longest_ps);
void pipe_cmd(tokenlist * input_tokens, pidlist * pids, time_t start, time_t longest_ps);
void double_pipe(tokenlist * input_tokens, pidlist *pids, time_t start,time_t longest_ps);
void sleep_exec(tokenlist * input_tokens, pidlist * pids, int pipe);
int check_pids(pidlist * pids);
void jobs(pidlist * pids);
void triple_pipe(tokenlist * input_tokens, pidlist *pids, time_t start, time_t longest_ps);
void copy_tokenlist(tokenlist* dest, tokenlist * src, int start, int end);
char * cm_path_cat(char * path, char * command);
int exec_tokenlist(char ** cmd_path,tokenlist * tokens, pidlist * pids, time_t start, time_t longest_ps);
int input_has_symbol(tokenlist * input, const char * symbol);

//Input Output redirection
//----------------------------
bool in = false;
bool out = false;
void ioRedirection(tokenlist *token, char * path, char * input, pidlist * pids, int background);
void containRedirection(tokenlist *tokens);
char * getFile(char * input, bool flag);

//TOKEN/PARSING RELATED DECLARATIONS
//--------------------------------------

char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *get_tokens_d(char *input, char delim);
tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);
void copy_tokenlist(tokenlist* dest, tokenlist * src, int start, int end);
void print_tokenlist(tokenlist* tokens, int start, int end);
void print_tokenlist_full(tokenlist* tokens);

//PID RELATED DELCARATIONS
//-------------------------------------

pidlist *new_pidlist(void);
pid * new_pid(int number, tokenlist * cmdline);
int add_pid(pidlist *pids, int number, tokenlist * cmdline);
void remove_pid(pidlist * pids, int index);
void free_pid(pid* ptr);
//CD RELATED DECLARATIONS
//------------------------------------
void cd_path(const char * arg);
void cd_home();
void cd(tokenlist * input);
void update_PWD();
//CONDITIONAL METHOD DECLARATIONS
//-------------------------------------
int cmd_has_slash(tokenlist * input); // returns 1 if first token contains a slash, 0 otherwise
int cmd_is_builtin(tokenlist * input); // returns 1 if first token refers to builtin func
//PATH ENV PARSING RELATED DECLARATIONS
//-----------------------------

tokenlist* get_PATH_tokens(); // gets PATH as tokens split around ':'
tokenlist* tokenize_path(char ** PATH); // actually tokenizes path
char * get_PATH_str(); // returns strcpy of PATH

//PATH SEARCHING RELATED DECLARATIONS
//-----------------------------
int search_directory(const char * filename, const char * pathdir); // returns 1 if in directory, 0 otherwise
char* get_path_dir(const char * filename); // returns cstring w/ path of dir containing filename
char * get_abs_path(const char* filename); // combines above two methods into one function
//PATH -> EXEC TRANSLATION DECLARATIONS
//-------------------------------------------
char * cm_path_cat(char * path, char * command);

void printPrompt();
void printingStuff(tokenlist *tokens, int check);

void getEnv(char * name);
bool containEnv(char *token);
void getTilde(char * n);
bool containTilde(char *token);

void parser();

int main()
{
	parser();
	return 0;
}

void parser()
{
	time_t start = time(NULL);
	time_t longest_ps = 0;
	
	pidlist * pids = new_pidlist();
	
	int exit = 0;
	
	while(exit != 1)
	{	
		//printf(">");
		printPrompt();
		in = false;		//both for IO redirection
		out = false;

		char * input_str = get_input();
		
		tokenlist * input_tokens = get_tokens_d(input_str, ' ');	//alt get tokens
		tokenlist *tokens = get_tokens(input_str);			//get tokens i/o
		
		int flag = -1;
		
		time_t ps_time = time(NULL);
		time_t bps_time = 0;
		
		char * cmd_path = get_abs_path(tokens-> items[0]);
		
		if(cmd_path != NULL)
			containRedirection(tokens);
		
		if(in || out)
		{
			int background = input_has_symbol(input_tokens, "&");
			ioRedirection(tokens, cmd_path, input_str, pids, background);
		}
		
		else	//if not i/o redirection, pipe and background process
		{
			if(input_has_symbol(input_tokens, "|") != -1)
				pipe_cmd(input_tokens, pids, start, longest_ps);
				
			else if(input_has_symbol(input_tokens, "&") != -1)
				sleep_exec(input_tokens, pids, 0);
			else
				exit = exec_tokenlist(&cmd_path,input_tokens, pids,start, longest_ps);

		}
		
		//checking for background processing 
		
		
		if(strcmp(input_tokens -> items[0], "jobs") != 0)
			bps_time = check_pids(pids);
			
		if(!strchr(input_str, '&'))
			ps_time = time(NULL) - ps_time;
		
		
		printf("\nbps_time = %ld", bps_time);
		printf("\nps_time = %ld", ps_time);
		
		if(bps_time > longest_ps)
			longest_ps = bps_time;
			
		if(ps_time > longest_ps)
			longest_ps = ps_time;
			
		printf("\nlongest =%ld", longest_ps);
		
		if(input_str != NULL)
			free(input_str);
			
		if(cmd_path != NULL)
			free(cmd_path);
		

		free_tokens(input_tokens);
	}
	free(pids);
}


//DEFINITIONS --------------------------------------------------------------------------------------------
tokenlist *get_tokens(char *input)
{
	char *buf = (char *) malloc(strlen(input) + 1);
	strcpy(buf, input);

	tokenlist *tokens = new_tokenlist();

	char *tok = strtok(buf, " ");
	while (tok != NULL) {
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}

	free(buf);
	return tokens;
}

void printPrompt()
{
	//creating the variables to be used in getenv
	char *user = "USER";
	char *machine = "MACHINE";
	char *pwd = "PWD";
	
	//capturing the return of getenv
	char *theUser = getenv(user);
	char *theMachine = getenv(machine);
	char *thePWD = getenv(pwd);

	printf("%s", theUser);		//user
	printf("%c", '@');			//@
	printf("%s", theMachine);	//machine
	printf("%s", " ");			//space
	printf("%c", ':');			//colon
	printf("%s", " ");			//space
	printf("%s", thePWD);		//working directory
	printf("%s", " ");			//space
	printf("%c", '>');			//arrow
	//format USER@MACHINE : PWD >
}

void update_PWD()
{
	char cwd[1000];
	getcwd(cwd, 1000);
	setenv("PWD",cwd, 1); 
}
void cd_path(const char * arg)
{
	if(strcmp(arg,"~") == 0)
		cd_home();
	else if(chdir(arg) != 0)
		perror(arg);
}
void cd_home() {
	chdir(getenv("HOME"));
}

void cd(tokenlist * input)
{
	if(input -> size > 2)
		printf("cd: Too many arguments.\n");
	else if(input -> size == 1)
		cd_home();
	else
		cd_path(input -> items[1]);

	update_PWD();		
}

pid * new_pid(int number, tokenlist * cmds)
{
	pid * new = (pid *) malloc(sizeof(pid));
	new -> number = number;
	new -> timer = 0;
	new -> cmdline = new_tokenlist();	
	copy_tokenlist(new -> cmdline, cmds, 0, cmds -> size);
	return new;	
}

int add_pid(pidlist * pids, int number, tokenlist * cmdline)
{
	int success = 0;
	if(pids -> size < 10)
	{
		success = 1;
		pids -> items[pids ->size] = new_pid(number,cmdline);	
		pids -> items[pids ->size]->timer = time(NULL);	//to set timer for background processing
		pids -> size += 1;
		printf("[%d] %d\n", pids -> size, number);
	}

	return success;	
}	
pidlist *new_pidlist(void)
{
	pidlist *pids = (pidlist *) malloc(sizeof(pidlist));
	pids->size = 0;
	return pids;
}

void remove_pid(pidlist *pids, int index)
{
	pid * ptr = pids ->items[index];
	free_pid(ptr);
	for(int i = index;i < pids ->size; i++)
		pids -> items[i] = pids -> items[i+1];
	pids -> size = pids -> size - 1;
}	

void free_pid( pid * ptr)
{
	if(ptr != NULL)	
	{
		free_tokens(ptr -> cmdline);
		free(ptr);
	}
	return;
}

void free_pids(pidlist* pids)
{
	for (int i = 0; i < pids->size; i++)
		free_pid(pids->items[i]);

	free(pids);
}
tokenlist *new_tokenlist(void)
{
	tokenlist *tokens = (tokenlist *) malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = (char **) malloc(sizeof(char *));
	tokens->items[0] = NULL; /* make NULL terminated */
	return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
	int i = tokens->size;

	tokens->items = (char **) realloc(tokens->items, (i + 2) * sizeof(char *));
	tokens->items[i] = (char *) malloc(strlen(item) + 1);
	tokens->items[i + 1] = NULL;
	strcpy(tokens->items[i], item);

	tokens->size += 1;
}


tokenlist *get_tokens_d(char *input, char delim)
{
	char *buf = (char *) malloc(strlen(input) + 1);
	strcpy(buf, input);

	tokenlist *tokens = new_tokenlist();
	char delimstr[2] = { delim, '\0' };	
	char *tok = strtok(buf, delimstr);
	while (tok != NULL) {
		add_token(tokens, tok);
		tok = strtok(NULL, delimstr);
	}

	free(buf);
	return tokens;
}

void free_tokens(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);

	free(tokens);
}

void copy_tokenlist(tokenlist* dest, tokenlist * src, int start, int end)
{
	if(start < 0) // fix bounds if they're invalid
		start = 0;
	if(end > src -> size)
		end = src -> size;
		
	for(int i = start; i < end; i++) // copy all tokens 
		add_token(dest, src->items[i]);	
	return;
}	

void print_tokenlist(tokenlist * tokens, int start, int end)
{
	int i = start;
	while (i < tokens -> size && i < end - 1)
	{
		printf("%s ", tokens -> items[i++]);
	}
	printf("%s\n", tokens -> items[end - 1]);
}

void print_tokenlist_full(tokenlist * tokens)
{
	for(int i = 0; i < tokens -> size - 1; i++)
	{
		printf("%s ", tokens -> items[i]);
	}
	printf("%s\n", tokens -> items[tokens -> size- 1]);
}
char *get_input(void)
{
	char *buffer = NULL;
	int bufsize = 0;

	char line[5];
	while (fgets(line, 5, stdin) != NULL) {
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;

		buffer = (char *) realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;

		if (newln != NULL)
			break;
	}

	buffer = (char *) realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;

	return buffer;
}


/*This function returns 1 if first token contains a slash, 0 otherwise*/ 
int cmd_has_slash(tokenlist * input)
{
	if(strchr(input -> items[0], '/' ) == NULL) // if no '/' found
		return 0; // return false
	else
		return 1; // return true
}
/*This function returns 1 if first token refers to a  builtin func
 * such as exit, cd, echo, or jobs
 */
int cmd_is_builtin(tokenlist * input)
{
	char * builtins[] = {"exit", "cd", "echo", "jobs"};
	int i = 0;
	while(i < 4) // 4 built in functions
	{
		if(strcmp(builtins[i], input -> items[0]) == 0) // if first token== a builtin
			return 1; //return true
		else
			i++; //check rest of list
	}
	return 0; // if this point is reached, it isn't a builtin
}
/* This function returns a char * to a copy of the PATH env variable*/
char * get_PATH_str()
{
	char * path_cpy = getenv("PATH");
	char * PATH = NULL;
	PATH = (char *)realloc( PATH, (strlen(path_cpy) +1) * sizeof(char));
	strcpy(PATH, path_cpy);
	return PATH;
}
/*This function returns a tokenlist of strings related to each
 * path in the PATH env variable, tokenized around the ':'
 */
tokenlist* tokenize_path(char ** PATH)
{
	tokenlist * t_list = new_tokenlist();
	t_list = get_tokens_d(*PATH, ':');
	return t_list ;
}
/*
 * Wrapper function that gets PATH, tokenizes it, and then returns
 * the tokenlist ptr
 */
tokenlist* get_PATH_tokens()
{
	char * PATH = get_PATH_str();
	tokenlist * t_list = tokenize_path(&PATH);
	free (PATH);
	return t_list;
} /*
 *	This function searches the directory specified by the 
 *	parameter path token (pathdir) for a file matching 
 *	the name "filename". If it is found, 1 is returned. If
 *	not, 0 is returned. 
 */

int search_directory(const char * filename, const char * pathdir)
{
	int found = 0;	// assume the file ISN'T in the directory 
	struct dirent * entry = NULL; // get the struct that tells you what is in the directory
	DIR * d = NULL; // directory pointer

	if(pathdir != NULL) // if the pathdir str isn't null
	{
		d = opendir(pathdir);	// open directory (similar to FILE *)	
		
		if(d != NULL)
		{	
			entry = readdir(d); 
			while(entry != NULL)  // while there are still items inside the directory, read them
			{
				if(strcmp(entry -> d_name, filename) == 0) // if the name of the item == filename
					found = 1;
				entry = readdir(d);
			}	
			closedir(d); // close directory
		}
	}
	else			// if the pathdir is NULL, report error	
	{
		perror("search_directory: Directory doesn't exist\n");
		return -1;
	}

	if(entry != NULL)
		free(entry);

	return found;
}

/*
 * This function returns a c-string representing the absolute path of the PATH dir
 * containing the filename parameter. If none is found, a NULL ptr is returned
 */
char * get_path_dir(const char * filename)
{	
	char * path = NULL; // assuming the directory isn't found
	tokenlist * path_tokens = get_PATH_tokens(); // get path tokens
	if(filename != NULL) // if filename parameter isn't NULL
	{
		int i = 0;
		int found = 0;
		do // look in all path dirs until filename is found
		{
			found = search_directory(filename, path_tokens -> items[i]); 
			if(found == 1) // if found, copy directory path
			{
				int strlength = strlen(path_tokens -> items[i]) + 1;
				path = (char*)calloc(strlength, sizeof(char));
				memcpy(path, path_tokens -> items[i], strlength);
			}	
			i++;
				
		}while(found != 1 && i < path_tokens -> size); // while file not found && paths remain

	}else			// print error if file not found
	{
		perror("get_path_dir: NULL filename parameter\n"); 		
	}
		free_tokens(path_tokens);
	return path;
}

/*
 * This function returns the absolute path of the filename IF found
 * within one of the PATH directories. If it doesn't find the path,
 * a NULL ptr is returned. Note that the return pointer needs to 
 * be freed later on if it isn't NULL.
 */
char* get_abs_path(const char * filename)
{
	char * path = get_path_dir(filename);
	if(path != NULL)
	{
		path = (char*)realloc(path, strlen(path) + strlen(filename) + 2);	
		strcat(path, "/");
		strcat(path, filename);
	}	
	return path;
}	

char * cm_path_cat(char * path, char * command)
{
	int length = strlen(path) + strlen(command) + 2;
	char * full = NULL;
	full = (char*)malloc(sizeof(char) * length);
	strcpy(full, path);
	strcat(full, "/");
	strcat(full,command);
	return full;
}

//DEFINITIONS

void ioRedirection(tokenlist *token, char * path, char * input, pidlist * pids ,int background)
{	
	char * file;
	tokenlist *copy = new_tokenlist();

	for(int i =0; i < token->size; i++)
	{
		if(strcmp(token->items[i], "<") != 0 && strcmp(token->items[i], ">") != 0)
			add_token(copy, token->items[i]);
		if(strcmp(token->items[i], "<") == 0 || strcmp(token->items[i], ">") == 0)
			break;
	}
	
	strcpy(copy->items[0], path);
	
	fflush(0);
	int fd0;
	int fd1;
		
	pid_t pid = fork();
	if(pid < 0)
	{
		printf("error");
	}
	if (pid == 0) 
	{
		if(in)
		{
			file = getFile(input, true);
			fd0 = open(file, O_RDONLY, 0644);
			if(fd0 < 0)
			{
				printf("error");
				exit(0);
			}
			dup2(fd0, 0);
			close(fd0);
		}
		if(out)
		{
			file = getFile(input, false);
			fd1 = open(file, O_CREAT | S_IRWXU | O_RDWR);
			if(fd1 < 0)
			{
				printf("error");
				exit(0);
			}
			dup2(fd1, 1);
			close(fd1);
		}
		execv(copy->items[0], copy->items);
		exit(1);
	} 
	else 
	{
		if(background != -1)
		{
			add_pid(pids,pid, token);
			free(copy);
			waitpid(pid, NULL, WNOHANG);
		}
		else
			waitpid(pid, NULL, 0);
	}
}
void containRedirection(tokenlist *tokens)
{
	for(int i = 0; i < tokens->size; i++)
	{
		if(!strcmp(">", tokens->items[i]))
			out = true;
		else if(!strcmp("<", tokens->items[i]))
			in = true;
	}
}
char * getFile(char *input, bool flag)
{
	char * n;
	char *copy = (char *) malloc(strlen(input) + 1);
	strcpy(copy, input);
	
	
	if(!flag)
	{
		n = strtok(copy, ">");
		n = strtok(NULL, "> ");
		
		if(strchr(input, '&'))
			n = strtok(n, "&");
	}
	else if(flag)
	{
		n = strtok(copy, "<");
		n = strtok(NULL, "< ");
		
		if(strchr(input, '&'))
			n = strtok(n, "&");
	}
	return n;
	
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
			printingStuff(tokens, 1);
		else if	(strcmp(cmd, "exit") == 0)
			exit_time = shell_exit(pids, start, longest_ps);
		else if	(strcmp(cmd, "jobs") == 0)
			jobs(pids);	
		else // must be cd
			cd(tokens);
	}
	else
	{ 	// if not builtin, fork and exec
		int pid = fork();
		if(pid == 0)
		{
			if(cmd_has_slash(tokens)) //cmd has path included
				execv(tokens -> items[0], tokens -> items);
				
			else 	// must determine path before exe
			{
				*cmd_path = get_abs_path(tokens->items[0]);
				if(*cmd_path == NULL)	//if no path, report err
				{
					printingStuff(tokens, 0);
					//printf("%s: Command not found\n",tokens -> items[0]);
				}
				else // execute cmd with abs path of executable
					execv(*cmd_path, tokens -> items);
			}
		}
		else
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
int check_pids(pidlist * pids)
{
	int pid = 0;
	int finish_time = 0;
	for(int i = 0; i < pids -> size; i++)	// iterate through stack
	{
		pid = pids -> items[i] -> number; // get pid number from ps
		if(waitpid(pid, NULL, WNOHANG) !=(pid_t)0)//if ps done
		{
			finish_time = time(NULL) - pids->items[i]->timer;
			printf("[%d]+ ",i + 1); //print out info
			print_tokenlist_full(pids ->items[i] -> cmdline);
			remove_pid(pids, i); // remove ps from stack
		}
		//else keep checking other processes in stack
	}
	return finish_time;
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
	}
	
	else	
	{
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

	}else if ((input_has_symbol(cmd2, "&")) != -1){ // if CMD2 is bckgr ps, add
		add_pid(pids, pid2, input_tokens); // pid with cmdline CMD1 |CMD 2&
	}
	
	close(p_fds[0]);
	close(p_fds[1]); // close off both ends of pipe

	waitpid(pid1, NULL, 0);
	if((input_has_symbol(cmd2, "&")) != -1) //if cmd2 is bckgrps, don't wait
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
	printf("Longest process ran for %ld sec\n", longest_ps);
	printf("Shell ran for %ld sec\n", shell_time);
	return 1;
}

/*
functions for echo, tilde, env variables
*/
void printingStuff(tokenlist *tokens, int check)
{
	
	if(check == 0)
	{
		if(containTilde(tokens->items[0]) || containEnv(tokens->items[0]))
			printf("command not found\n");
		else 
			printf("command not found: %s\n", tokens->items[0]);
		
	}
		
			
	for(int i = check; i < tokens->size; i++)
	{
		if(containTilde(tokens->items[i]))
			getTilde(tokens->items[i]);
		
		else if(containEnv(tokens->items[i]))
			getEnv(tokens->items[i]);

		else if(check == 1)
			printf("%s ", tokens->items[i]);
	}
	
	printf("\n");
}

void getEnv(char * n)
{
	if(n[0] != '$')
	{	
		n = strtok(n, "$");
		printf("%s", n);
		n = strtok(NULL, "$");
	}
	else
		n = strtok(n, "$");

	printf("%s",getenv(n));
}

void getTilde(char * n)
{
	n = strtok(n, "~");
	printf("%s",getenv("HOME"));
	
	if(n == NULL)
		return;
		
	if(containEnv(n))
		getEnv(n);
	else
		printf("%s", n);
}

bool containEnv(char *token)
{
	while(*token)
	{
		if (strchr("$", *token))
			return true;
		token++;
	}
	return false;
}

bool containTilde(char *token)
{
	while(*token)
	{
		if (strchr("~", *token))
			return true;
		token++;
	}
	return false;
}
