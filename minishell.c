#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

//limits
#define MAX_TOKENS 100
#define MAX_STRING_LEN 100

size_t MAX_LINE_LEN = 10000;


// builtin commands
#define EXIT_STR "exit"
#define EXIT_CMD 0
#define UNKNOWN_CMD 99


FILE *fp; // file struct for stdin
char **tokens;
char *line;

//  for listing jobs
char *proc_history[100] ;
int proc_historyLength = 0;
int proc_historypid[100] ;
int token_count = 0;


void initialize()
{

	// allocate space for the whole line
	assert( (line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);

	// allocate space for individual tokens
	assert( (tokens = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);

	// open stdin as a file pointer 
	assert( (fp = fdopen(STDIN_FILENO, "r")) != NULL);

}

void tokenize (char * string)
{
	
	int size = MAX_TOKENS;
	char *this_token;
	int i=0;
	for(i=0;i<token_count;i++)
	{
		tokens[token_count] = NULL;
	}

	token_count =0;
	while ( (this_token = strsep( &string, " \t\v\f\n\r")) != NULL) {

		if (*this_token == '\0') continue;
		
		tokens[token_count] = this_token;

		printf("Token %d: %s\n", token_count, tokens[token_count]);

		token_count++;

		// if there are more tokens than space ,reallocate more space
		if(token_count >= size){
			size*=2;

			assert ( (tokens = realloc(tokens, sizeof(char*) * size)) != NULL);
		}
	}

	tokens[token_count] = NULL; // execvp expects a NULL at the end

}

void read_command() 
{
	// getline will reallocate if input exceeds max length
	assert( getline(&line, &MAX_LINE_LEN, fp) > -1); 

	printf("Shell read this line: %s\n", line);

	tokenize(line);
}
void run_execprocc(){

	pid_t pid;
	char * status = "RUNNING";
	int bg_proc;
	int status_c;

	 if(strcmp(tokens[0], "fg" ) == 0){
		printf("Foreground Process %d \n", atoi(tokens[1]));
		
		if (waitpid((__pid_t) atoi(tokens[1]), &status_c, 0) < 0)
        {

            perror("foreground process() error");
            exit(UNKNOWN_CMD);
            
        }
	 } else if (strcmp(tokens[0], "listjobs") == 0) {
		 	int status;
			int ret;
			char * status_process;
            printf("List of process:\n");
            for (int i=0;i < proc_historyLength ;i++)
            {
				ret = waitpid(proc_historypid[i], &status, WUNTRACED);
				if (status == -1) {
					status_process = "ERROR";
				}
				else if (status == 0) {
					status_process = "RUNNING";
				} 
				else{
					status_process = "FINISHED";
				}

				
                 printf(" %s with PID %d Status: %s\n", proc_history[i],proc_historypid[i],status_process );
            }
           
         }
		 else {
        if ((bg_proc = ( * tokens[token_count - 1] == '&')) != 0) {
            tokens[--token_count] = NULL;
       		 }
          

    // fork for exec
		pid = fork();
        if ((pid) == 0)
        {
               if (execvp(tokens[0], tokens) < 0) 
            {
                printf("%s: Command not found.\n", tokens[0]);
                exit(EXIT_CMD);
            }
        }
		proc_history[proc_historyLength] = strdup(line);              
        proc_historypid[proc_historyLength++] = pid;
        if (!bg_proc) {
            printf("pid: %d \n", pid);
            //int status_c;
            if (waitpid(pid, & status_c, 0) < 0)
            {
                fprintf(stderr, "Error in Running Foreground Process  \n");
                exit(UNKNOWN_CMD);
            }
        }
    }
}
int run_command() {

	if (strcmp(tokens[0], EXIT_STR ) == 0)
		return EXIT_CMD;
	else
		
		run_execprocc();

	return UNKNOWN_CMD;
}

int main()
{
	
	initialize();

	do {
		printf("sh550> ");
		read_command();
		
	} while( run_command() != EXIT_CMD );

	return 0;
}
