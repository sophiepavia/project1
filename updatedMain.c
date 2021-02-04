#define _POSIX_C_SOURCE  200809L // necessary for setenv
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h> // for PATH searching
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

void parser();

typedef struct 
{
	int size;
	char **items;
} tokenlist;

//void stringCompare(char *input);
//void exitFunction();
void cdFunction();

void printPrompt(void);

char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

void getEnv(char * name);
bool containEnv(char *token);
void getTilde(char * n);
bool containTilde(char *token);
void printingStuff(tokenlist *tokens, int check);

//Input Output redirection
//----------------------------
void inputOut(char *left, char *right, int flag, char *path);
void parseIO(char *input, int flag, char *path);
void externalCommand2(char *input, char *path);
int isInputOut(char *token);

void externalCommand(tokenlist *token, char *path);

//CD IMPLEMENTATION DECLARATIONS
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

tokenlist *get_tokens_d(char *input, char delim);// alternate version of get_tokens

tokenlist* get_PATH_tokens(); // gets PATH as tokens split around ':'
tokenlist* tokenize_path(char ** PATH); // actually tokenizes path
char * get_PATH(); // returns copy of PATH env var cstring

//PATH SEARCHING RELATED DECLARATIONS
//-----------------------------
int search_directory(const char * filename, const char * pathdir); // returns 1 if in directory, 0 otherwise
char* get_path_dir(const char * filename); // returns cstring w/ path of dir containing filename
char * get_abs_path(const char* filename); // combines above two methods into one function


int main()
{
	parser();
	return 0;
}
void parser(void)
{
	while (1) {
		printPrompt();
		
		char *input = get_input();
		//printf("whole input: %s\n", input);
		
		tokenlist *tokens = get_tokens(input);
		//for (int i = 0; i < tokens->size; i++) {
		//	printf("token %d: (%s)\n", i, tokens->items[i]);
		//}
		/*
		check for built in instead of direct cd
		*/
		char * command_path = get_abs_path(tokens-> items[0]);
		
		//------ I/O --------
		int flag = -1;
		for(int i=0; i < tokens-> size; i++)
		{
			flag = isInputOut(tokens-> items[i]);
			if(flag == 1)
			{
				//1 means output > so we need path of
				//left side of >
				char *path = get_abs_path(tokens-> items[0]);
				parseIO(input, flag, path);
			}	
			else if(flag == 2)
			{
				//2 means input < so we need path of
				//right side of <
				char *path = get_abs_path(tokens-> items[2]);
				parseIO(input, flag, path);
			}
		}
		
		if(cmd_is_builtin(tokens))	
		{
			if(strcmp(tokens->items[0], "cd") == 0)
				cd(tokens);
			//echo
			else if(strcmp(tokens->items[0], "echo") == 0)
			{
				printingStuff(tokens, 1);
			}
			//jobs
			//exit
		}

		else if(command_path != NULL && flag == -1)
		{
			externalCommand(tokens, command_path);
			free(command_path);
		}
		else
		{
			printingStuff(tokens, 0);
		}
			
		free(input);
		free_tokens(tokens);
	}
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

void free_tokens(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);

	free(tokens);
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
void externalCommand(tokenlist *token, char *path)
{
	//need to have an arr which is null terminated and path is first slot
	//copy token list to an arr

	strcpy(token->items[0], path);

	int pid = fork();
	if(pid == 0)
	{
		execv(token->items[0], token->items);
	}
	else
	{
		waitpid(pid, NULL,0);
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

/* Alternate version of get_tokens from parser.c; allows user 
 * to pass in the delimiter char for the strtok function for use
 * in parsing the PATH env, etc.
 */
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
/* This function returns a char * to a copy of the PATH env variable*/
char * get_PATH()
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
	char * PATH = get_PATH();
	tokenlist * t_list = tokenize_path(&PATH);
	free (PATH);
	return t_list;
}

/*
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

void printPrompt(void)
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

void cd_home()
{
	chdir(getenv("HOME"));
}

void cd(tokenlist * input)
{
	if(input->size > 2)
		printf("cd: Too many arguments.\n");
	else if(input->size == 1)
		cd_home();
	else
		cd_path(input -> items[1]);

	update_PWD();		
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

void printingStuff(tokenlist *tokens, int check)
{
	char flag = 0;
	for(int i = check; i < tokens->size; i++)
	{
		if(containTilde(tokens->items[i]))
			getTilde(tokens->items[i]);
		
		else if(containEnv(tokens->items[i]))
			getEnv(tokens->items[i]);

		else if(check == 1)
			printf("%s ", tokens->items[i]);
	}
	//if(check == 0)
	//	printf("\nInvalid command");
	
	printf("\n");

}

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
void parseIO(char *input, int flag, char *path)
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
		printf("Path of the right side is: %s", path);
	}
	else if(flag == 1)
	tokens = get_tokens_d(input, '>');
	
	char *right = strtok(tokens-> items[1], " ");
			//left				//right	
	inputOut(tokens-> items[0], right, flag, path);
	free_tokens(tokens);

}	

void inputOut(char *left, char *right, int flag, char *path)
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
			waitpid(pid, NULL, 0);
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
				printf("Path in the function is: %s\n", path);
				externalCommand2(right, path);
				free(path);
			}
		}
		else if(pid != 0)
		{
			waitpid(pid, NULL, 0);
		}
	}
}