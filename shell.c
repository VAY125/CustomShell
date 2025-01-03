/*  This is a Custom UNIX shell by Vivek Yadav, 
	
	Features supported are :
		1. Command Line Editing (Only backspace is supported, left and right cursor movement is not yet supported)
		2. Command Piping (Command Piping upto 5 commands is supported, which can be extended with not much effort)
		3. Getting/Setting Environment Variables
		4. Essential Builtin Commands (help, exit, history, cd)
		5. I/O redirection (Only > and < are supported, which can be extended if required)
		6. Wildcard support, Substituting variable using $ within command, $(command) for command substitution
*/

#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<termios.h>
#include<wordexp.h>

#define TOK_DELIM " \t\r\n\a"        // It is used to separate command and args from input

short history_count=0;
char **history;						// Stores previous commands
char *files[5];						// Stores filenames for I/O redirection
char *piped[5];						// Stores piped commands
int piped_count = 0;				// Piped Command number



// This function is run before every input line to reset global variables

void start_routine(void){

	// getcwd is used to get the current directory name from Environment Variables

	char *pwd = getcwd(NULL, 30);

	piped_count = 0;
	printf("%s $ ", pwd);

	for(int i=0; i<5; i++){
		piped[i]=NULL;
		files[i]=NULL;
	}

	free(pwd);
		return;
}


// This function is used to take the input and separate piped commands

void take_line(){
	char *line, *found;

	line = (char *)malloc(100*sizeof(char));

	int i=0;
	char c;
	while(1){
		c = getchar();
		if(c==EOF || c=='\n'){
			history[history_count] = line;
			history_count = (history_count+1)%10;
			break;
		}
		else line[i] = c;

		i++;
	}

	i=0;

	while((found = strsep(&line, "|")) != NULL){

		piped[i] = found;
		i++;
	}
	free(line);
	return;
}


// This function is used for pattern matching and substituting variables and commands

char **regex_matching(char **parsed, int count){

	wordexp_t expanded;		// wordexp struct which stores size of char** array, char * array and offset
	expanded.we_offs = count;	//offset after which strings are copied into expanded's string array

	int val = wordexp(parsed[count], &expanded, WRDE_DOOFFS);	// WRDE_DOOFS flag is used for offests, parsed[count] is the pattern to be matched

	if(val!=0){
		fprintf(stderr, "Error : Pattern Matching Error\n");
		return NULL;
	}

	for(int i=0; i<count; i++){
		expanded.we_wordv[i] = parsed[i];	// Copying offset strings into expanded's string array
	}

	free(parsed);

	return expanded.we_wordv;
}


// After taking the input and separating piped commands, this function parse the command from its arguments
// it also separates the filename for I/O redirection

char **parsing(char *line){
	char **parsed = (char **)malloc(50*sizeof(char *));
	char *token;
	int i=0;

	token = strtok(line, TOK_DELIM);

	while(token!=NULL){
		if(*token=='<'){
			token = strtok(NULL, TOK_DELIM);
			files[0] = token;
		}
		else if(*token=='>'){
			token = strtok(NULL, TOK_DELIM);
			files[1] = token;
		}
		else{
			parsed[i] = token;
			i++;
		}
		token = strtok(NULL, TOK_DELIM);
	}
	return regex_matching(parsed, i-1);
}


// This function is used to check whether the given command is a builtin command or not

int check_builtin(char *command){
	if(!strcmp(command, "cd") || !strcmp(command, "history") || !strcmp(command, "exit") || !strcmp(command, "help") || !strcmp(command, "setenv")) return 1;
	return 0;
}



// If the command is a builting command, this function executes it

int builtins(char **args){
	if(strcmp(args[0], "cd")==0){
		if(args[1]==NULL){
			if(chdir(getenv("HOME"))!=0) return -1;
			printf("Current directory is changed to : %s\n", getenv("HOME"));
		}
		else{

			if(chdir(args[1])!=0) {
				fprintf(stderr, "Error : Due to some error current directory wasn't changed\n");
				return 1;
			}

			char *pwd = (char *)malloc(30*sizeof(char));
			pwd = getcwd(NULL, 30);

			printf("Current directory is changed to : %s\n", pwd);

			free(pwd);
		}
	}

	else if(strcmp(args[0], "history")==0){
		for(int i=0; i<10; i++){
			if(history[(history_count-1-i)%10]!=NULL) 
				printf("%d. %s\n",i, history[(history_count-1-i)%10]);
			else break;
		}
		return 1;
	}

	else if(strcmp(args[0], "exit")==0){
		return 0;
	}

	else if(!strcmp(args[0], "help")){
		printf("\n$ This is Vivek's Shell $\n");
		printf("Following commands are built in:\n");
		printf("1. cd\n2. help\n3. exit\n4. history\n5. setenv (format : $ setenv variable_name=variable_value)\n");
		printf("For any other information please refer to man page\n");
	}
	
	else if(!strcmp(args[0], "setenv")){
		int val = putenv(args[1]);
		if(val!=0){
			fprintf(stderr, "Error : Couldn't set env variable\n");
		}
	}
	return 1;
}


// After the command is parsed, this function is used to execute the command

int execute(char **parsed){
	if(check_builtin(parsed[0])) return builtins(parsed);	

	pid_t pid, wpid;
	int status;

	pid = fork();

	if(pid<0) fprintf(stderr, "Error : forking error\n");

	else if(pid==0){	// For child process
		if(files[1]!=NULL){
			freopen(files[1], "w", stdout);		// Output Redirection
		}
		if(files[0]!=NULL){
			freopen(files[0], "r", stdin);		// Input Redirection
		}

		if(execvp(parsed[0], parsed)==-1){
			fprintf(stderr, "Error : error in executing\n");
			exit(0);
		}
	}

	else{		// For Parent Process
		
			wpid = waitpid(pid, &status, WUNTRACED);	// Waiting for child to finish
	} 
	return 1;
}


int main(){
	char *user = getenv("USER");
	printf("User is %s \n", user);
	history = (char **)malloc(11*sizeof(char *));

	while(1){

		start_routine();

		take_line();

		while(piped[piped_count] != NULL){
			char *entered_command = piped[piped_count];

			if(piped_count > 0) files[0] = "input.txt";

			if(piped[++piped_count] != NULL) files[1] = "input.txt";
			else files[1] = NULL;

			char **parsed_command = parsing(entered_command);
			if(parsed_command==NULL) continue;

			int exec_status = execute(parsed_command);

			if(!exec_status){
				printf("\nThank you for using this $hell!\n");
				return 0;
			}
		}
		printf("\n");
	}
	free(history);
	return 0;
}