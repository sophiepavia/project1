#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>

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
void lsFunction();

char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

void getEnv(const char * name);
bool containEnv(char *token);

int main()
{
	parser();
	return 0;
}
void parser(void)
{
	while (1) {
		printPrompt();
		//printf("> ");
		
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
			else if(strchr("~", *tokens->items[i]))
			{
				printTilde(tokens->items[i]);
			}
			else if((strcmp(tokens->items[0], "ls") == 0)) 
			{
				lsFunction();
				//this is the path search version of ls
			}
		}
		stringCompare(input);
		/*
		
		if(strcmp(tokens->items[0], "(echo)"))
		{
			for(int i = 1; i < tokens->size; i++)
			{
				if(containEnv(tokens->items[i]))
				{
					getEnv(tokens->items[i]);
				}
				else
					printf("%s ", tokens->items[i]);
			}
			printf("\n");
		}
		*/
	
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
/*void getEnv(const char * name)
{
	const char *n = name;
	n++;
	printf("%s",getenv(n));}
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
*/

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
	//functionality may not be completely correct, need to read
	//more about tilde and look into it further
}

void lsFunction()
{
	char *path = "PATH";
	char *thePath = getenv(path);
	char *x[2];
	x[0] = "/bin/ls";
	x[1] = NULL;
	
	for(int i=0; i < strlen(thePath); i++)
	{
		if(thePath[i] == ':')
			printf("%c", '\n');
		else 
			printf("%c", thePath[i]);
	}

	int pid = fork();
	if(pid == 0)
	{
		execv(x[0], x);
	}
	else 
		waitpid(pid, NULL, 0);
	//still working on what arguments need
	//to be passed to execv()
}
void stringCompare(char *input)
{
	char a[5] = {'e', 'x', 'i', 't', '\0'};
	char b[3] = {'c', 'd', '\0'};
	
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
