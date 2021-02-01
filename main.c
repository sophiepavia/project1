#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h> // for PATH searching
#include <unistd.h>

void parser();

typedef struct 
{
	int size;
	char **items;
} tokenlist;

void stringCompare(char *input);
void exitFunction();
void cdFunction();

char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

void getEnv(char * name);
bool containEnv(char *token);
void getTilde(char * n);
bool containTilde(char *token);

void externalCommand(tokenlist *token, char *path);

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
char * get_path(const char* filename); // combines above two methods into one function


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
		char * command_path = get_path(tokens-> items[0]);
		if(command_path != NULL)
		{
			//printf("command %s located at %s\n", tokens->items[0], command_path);
			externalCommand(tokens, command_path);
			free(command_path);
		}
				
		//stringCompare(input);
		if(strcmp(tokens->items[0], "echo") == 0)
		{
			for(int i = 1; i < tokens->size; i++)
			{
				if(containTilde(tokens->items[i]))
					getTilde(tokens->items[i]);
				
				else if(containEnv(tokens->items[i]))
					getEnv(tokens->items[i]);
		
				else
					printf("%s ", tokens->items[i]);
			}
			printf("\n");
		}
		else
		{
			for(int i = 0; i < tokens->size; i++)
			{
				if(containTilde(tokens->items[i]))
				{
					getTilde(tokens->items[i]);
					printf("\n");
				}
				if(containEnv(tokens->items[i]))
				{
					getEnv(tokens->items[i]);
					printf("\n");
				}
				
			}
		
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
char* get_path(const char * filename)
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