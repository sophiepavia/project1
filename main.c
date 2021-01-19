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
void findPath(const char *copyInput);

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
		printf("> ");
		
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
		}

		
		//stringCompare(input);
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
void getEnv(const char * name)
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

void findEnv(const char *input)
{
	//prints the expansion of the env variable
	//if the env variable is PATH then it jumps to the 
	//findPath function
	
	const char *copyInput = input;
	copyInput++;
	
	if(strcmp(copyInput, "PATH") == 0)
	{
		findPath(copyInput);
	}
	
	else
	printf("%s", getenv(copyInput));
}
void findPath(const char *copyInput)
{
	char *path = getenv(copyInput);
	//parsing the enviromental varaible path
	
	for(int i = 0; i < strlen(path); i++)
	{
		if(path[i] == ':')
		{
			printf("%c", '\n');
			//a colon deliminates one path from another
			//if one is detected a new line will be 
			//printed and the colon is ignored
		}
		else
		{
			printf("%c", path[i]);
			//prints the paths character by character
		}
	}
}

/*
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
*/
