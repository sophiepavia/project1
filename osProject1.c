#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>

void parser();

typedef struct 
{
	int size;
	char **items;
} tokenlist;

void stringCompare(char *input);
void exitFunction();
void cdFunction();
void findEnv(const char *input);
void printPrompt(void);
void printTilde(const char *input);
void inputOut(char *left, char *right, int flag);

bool isInputOut(tokenlist *tokens);

//Path searching
tokenlist *getDelim(char *input, char delim);
tokenlist *tokenizingPath(void);
char *findPath(char *filename);


char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

int main()
{
	parser();
	return 0;
}
void parser(void)
{
	while (1) {
		printPrompt();
		
		/* input contains the whole command
		* tokens contains substrings from input split by spaces
		*/
		
		char *input = get_input();
		//printf("whole input: %s\n", input);
		
		tokenlist *tokens = get_tokens(input);
		for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]);
		}
		
		for(int i = 0; i < tokens->size; i++)
		{
			if(strchr("$", *tokens->items[i]))
			{
				//if the token begins with a $ then it is an
				//enviromental variable and needs to jump to 
				//finEnv function
				findEnv(tokens->items[i]);
			}
			else if(strchr("~", *tokens->items[0]))
			{
				printTilde(tokens->items[0]);
			}
			else if((strcmp(tokens->items[0], "ls") == 0)) 
			{
				char *input = "ls";
				findPath(input);
					
			}
			else if(isInputOut(tokens))
			{
				//if the input redirects such as
				//< or > we go to this file
				//to execute the command
			}
		}
		stringCompare(input);
		
		printf("%c", '\n');
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
void stringCompare(char *input)
{
	char a[5] = "exit\0";
	char b[3] = "cd\0";
	
	if(strcmp(a,input) == 0)
	return exitFunction();

	else if(strcmp(b,input) == 0)
	return cdFunction();	
}
void exitFunction(void)
{
	//must do a check to see if background proccesses
	//are running, print how long they are running,
	//Print how long it took for the longest running 
	//command to execute then exit
	//if no processes are running then exit
	
	exit(0);
}
void cdFunction(void)
{
	//setting up the change directory
	//function, will implement later
}

void findEnv(const char *input)
{
	//prints the expansion of the env variable
	const char *copyInput = input;
	copyInput++;
	
	printf("%s", getenv(copyInput));
	printf("%s", " ");
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
void printTilde(const char *input)
{
	char *home = "HOME";
	char *pwd = "PWD";
	char *oldPWD = "OLDPWD";
	char *theOldPWD = getenv(oldPWD);
	char *thePWD = getenv(pwd);
	char *theHome = getenv(home);
	const char *copyInput = input;

	//this is if the command was ~+
	if(input[1] == '+')	
	{
		printf("%s", thePWD);
		for(int i=2; i < strlen(copyInput); i++)
		printf("%c", copyInput[i]);
		//printing the expression entered after ~/

	}
	//this is if the command was ~-
	else if(input[1] == '-')
	{
		printf("%s", theOldPWD);
		for(int i=2; i < strlen(copyInput); i++)
		printf("%c", copyInput[i]);
		//printing the expression entered after ~/
	}
	
	else
	{
		//this is if the command was ~/
		printf("%s", theHome); 
	
		for(int i=1; i < strlen(copyInput); i++)
			printf("%c", copyInput[i]);
		//printing the expression entered after ~+/
	}
}

tokenlist *getDelim(char *input, char delim)
{
	//exact same function as for parsing with get_tokens
	//but now what the parser deliminates on is not
	//static but can change, this is ideal for path searching
	
	char *buf = (char *) malloc(strlen(input) + 1);
	strcpy(buf, input);
	
	tokenlist *tokens = new_tokenlist();
	char d[2] = {delim, '\0'};
	char *tok = strtok(buf, d);
	while (tok != NULL) {
		add_token(tokens, tok);
		tok = strtok(NULL, d);
	}
	
	free(buf);
	return tokens;
}

tokenlist *tokenizingPath(void)
{
//	char *path = "PATH";
//	char *thePath = getenv(path);					//gets the path
	
	char * path_cpy = getenv("PATH");
	char * PATH = NULL;
	PATH = (char *)realloc( PATH, (strlen(path_cpy) +1) * sizeof(char));
	strcpy(PATH, path_cpy);
	
	tokenlist *tokenizedPath = new_tokenlist();	//creates a new token list
	tokenizedPath = getDelim(PATH, ':');		//tokenizes the list based on a colon
							
	//&thePath
	free(PATH);
	return tokenizedPath;
	
}

char *findPath(char *filename)
{
	tokenlist *tokenizedPath = tokenizingPath();	
	//getting the return value of the function
	//which is a tokenized list of all the paths
	//found using the getDelim function which
	//parses with respect to each colon :
	
	for(int i=0; i < tokenizedPath ->size; i++)
	{
		strcat(tokenizedPath-> items[i], "/");
		strcat(tokenizedPath-> items[i], filename);
	}
		/*
	DIR *ptr = NULL; 					//pointer to directory 
	struct dirent *directory = NULL;	//directory struct
	
	for(int i=0; i < tokenizedPath -> size; i++)
	{
		//opening directory of path being searched
		ptr = opendir(tokenizedPath -> items[i]);
		directory = readdir(ptr);
		
		//printf("%s", tokenizedPath -> items[i]);
		//printf("%c", '\n');
		
		if(directory != NULL && ptr != NULL) 
		{
			//if the directory we are looking for is found
			if(strcmp(directory-> d_name, filename) == 0)
			{
				foundPath = tokenizedPath -> items[i];
				//strlength = strlen(tokenizedPath -> items[i]) + 1;
				//foundPath = (char*)calloc(strlength, sizeof(char));
				//memcpy(foundPath, tokenizedPath -> items[i], strlength);
				printf("%s", tokenizedPath -> items[i]);
			}
		}
		closedir(ptr);
		if(directory == NULL)
		{
			printf("%s", "No such file or directory '\n");
			break;
			//if the directory is invalid exit the loop
		}
	}	

	printf("%s", foundPath);
	free_tokens(tokenizedPath);
	free(foundPath);
	*/
	return 0;
}
	
bool isInputOut(tokenlist *tokens)
{
	char *left = NULL;
	char *right = NULL;
	int flag = 0;
	//this is making sure that if file 
	//redirection output is happening then
	//the file being written from is valid
	if(strcmp(tokens-> items[1], ">") == 0)
	{	if(tokens-> items[0] != NULL)
		{
			left = tokens-> items[0];
			right = tokens -> items[2];
			flag = 1;
			inputOut(left, right, flag);
			return true;
		}
	}
	//this checks for file redirection input
	//and makes sure both files are valid
	else if(strcmp(tokens-> items[1], "<") == 0)
	{
		if(tokens-> items[0] != NULL && 
			tokens-> items[2] != NULL)
		{
			left = tokens-> items[0];
			right = tokens -> items[2];
			flag = 2;
			inputOut(left, right, flag);
			return true;
		}
	}
	else return false;
}
void inputOut(char *left, char *right, int flag)
{
	//if flag = 1 then we have output >
	//if flag = 2 then we have input <
	
	if(flag == 1)
	{
		int fd = open(right, O_RDWR | O_CREAT);
		pid_t pid = fork();
		if(pid == 0)
		{
			close(1); 	//closing stdout
			dup(fd);	//duplicate the file descriptor
			close(fd);	//closing original file descriptor
						//call to external command execution with ls
		}
		else
		{
			close(fd);
			waitpid(pid, 0, 0);
		}
	}
	else if(flag == 2)
	{
		int fd = open(left, O_RDWR | O_CREAT);
		pid_t pid = fork();
		if(pid == 0)
		{
			close(0);
			dup(fd);
			close(fd);
		}
		else
		{
			close(fd);
			waitpid(pid, 0, 0);
		}
	}
}