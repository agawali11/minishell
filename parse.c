#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>  
#include <fcntl.h> 

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

// Pipe Left and Right Part
char **left_arr;
char **right_arr;

void initialize()
{

	// allocate space for the whole line
	assert( (line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);

	// allocate space for individual tokens
	assert( (tokens = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);

	// open stdin as a file pointer 
	assert( (fp = fdopen(STDIN_FILENO, "r")) != NULL);

	// allocate space for pipe left part
	assert( (left_arr = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);

	// allocate space for pipe left part
	assert( (right_arr = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);

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

	//	printf("Token %d: %s\n", token_count, tokens[token_count]);

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

	//printf("Shell read this line: %s\n", line);
	//printf("Shell Start\n");

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

            perror("ERROR:foreground process");
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
           
         }else {
        if ((bg_proc = ( * tokens[token_count - 1] == '&')) != 0) {
            tokens[--token_count] = NULL;
       		 }
		 	
		// fork for exec
		pid = fork();
        if ((pid) == 0)
        {
            //printf("I am here 1.\n");
            //printf("I am here token[0]:%s.\n", tokens[0]);
             
			
		//Redirection of Input and Output stream
		//int tk = 0;
		int tk = 0, fd;
		//loop < token counts
		for(tk = 0; tk < token_count; tk++)
		{
			//printf("I am here 2.\n");
			if(strncmp(tokens[tk],"|",1)==0)
			{
				int ck = 0;
				//printf("I am here 3.\n");
				for(int k = 0; k < token_count; k++)
				{
					//int t;
					if(strncmp(tokens[k],"|",1)==0)
					{
						for(int t = 0; t< k;t++)
						{
							memmove(&left_arr[t],&tokens[t],sizeof(char *)); // left part of pipe

						}
						for(int r=k+1; r<=token_count;r++)
						{
							// right part of pipe
							memmove(&right_arr[ck],&tokens[r],sizeof(char *));	 
							ck++;
						}
					}
				}


				for(int p=0; p< token_count;p++)
				{
					//printf("I am here 5.\n");
					int file_descriptors[2];
					if(pipe(file_descriptors) == -1) 
					{
						perror("pipe");
						exit(1);
					}
					pid_t ppid = fork();
					if (ppid == -1) 
					{ //error
						printf("ERROR:fork!!\n");
						//return 1;
					}
					if(ppid==0) // first
					{			
							//printf("I am here 6.\n");
							close(1);//close stdout
							dup(file_descriptors[1]);//duplicate write end
							close(file_descriptors[0]);//close read end
							close(file_descriptors[1]);
							if(execvp(left_arr[0], left_arr)==-1);  
							{
								printf("EXEC ERROR\n");      
								exit(1);
							}
					}
					else if(ppid>0) // last
					{	int k=ck-1;
						
						
						close(0);//close stdin
						dup(file_descriptors[0]);//duplicate read end
						close(file_descriptors[0]);//close 
						close(file_descriptors[1]);//close write end
						//printf("I am here 7.\n");
						for(int h=0;h<k;h++)//check for io redirection for last command
						{
							//printf("\n right arr char:%s",right_arr[h]);
							if(strcmp(right_arr[h],"<")==0)
							{
								if (right_arr[h+1] == NULL)
								{
									printf("no arguments\n");
								}
								right_arr[h] = NULL;
								fd = open(right_arr[h+1], O_RDONLY);  
								if(dup2(fd,0)<0)
									perror("ERROR: Duplication");
								close(fd);
								
							}
							if(strcmp(right_arr[h],">")==0)
							{
								if (right_arr[h+1] == NULL)
								{
									printf("no arguments\n");
								}
								right_arr[h] = NULL;
								fd = open(right_arr[h+1], O_CREAT | O_TRUNC | O_WRONLY,0600); 
								if(dup2(fd,1)<0)
									perror("ERROR: Duplication");
								close(fd);
							}
						}
						
						if(execvp(right_arr[0], right_arr)==-1);  
						{
							printf("EXEC ERROR\n");      
							exit(1);
						}
					}					
				}
			}


            // printf("I am here 2.\n");
			// int i=0,fd;
            // printf("I am here 3.\n");
			else if(strncmp(tokens[tk],"<",1)==0)
			{
               // printf("I am here 4.\n");
				if (tokens[tk+1] == NULL)
				{
                    printf("No Arguments\n");
				}
				tokens[tk] = NULL;
				fd = open(tokens[tk+1], O_RDONLY); // open a file descriptor to read
				if(dup2(fd,0)<0)
				perror("ERROR: Duplication");
				close(fd);
				
			}
			else if(strncmp(tokens[tk],">",1)==0)
			{
				if (tokens[tk+1] == NULL)
				{
                    printf("No Arguments\n");
				}
				tokens[tk] = NULL;
				fd = open(tokens[tk+1], O_CREAT | O_TRUNC | O_WRONLY,0600); // open a file descriptor to write
				if(dup2(fd,1)<0)
					perror("ERROR: Duplication");
				close(fd);
			}
          
        }
			 if ((bg_proc = ( * tokens[token_count - 1] == '&')) != 0) {
            tokens[--token_count] = NULL;
			setpgid(0,0);
			printf( "Process %d is in background mode \n",getpid());
       		 }
			 if (execvp(tokens[0], tokens) < 0) 
            {
                printf("%s: Command not found.\n", tokens[0]);
                exit(1);
            }
			exit(EXIT_CMD);
        }
			
		//setpgid(0,0);
		 	proc_history[proc_historyLength] = strdup(line);              
			proc_historypid[proc_historyLength++] = pid;
			
		
        if (!bg_proc) {
            //printf("pid: %d \n", pid);
            //int status_c;
            if (waitpid(pid, & status_c, 0) < 0)
            {
                fprintf(stderr, "ERROR in Running foreground Process  \n");
                exit(UNKNOWN_CMD);
            }
        }
    }
}

int run_command() {

	if (strcmp(tokens[0], EXIT_STR ) == 0){
		waitpid(-1, NULL, WNOHANG);
		return EXIT_CMD;
	}
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
