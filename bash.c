#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>


/*function declarations section */
int readline(int fd, char* buf_str, size_t max);//function by : Dr. Stefan Bruda
char** tokenize(char* input);//function organised from web reference and strtok() man pages
void fetch_config();
void do_more(char* file_name);
void print_err_msg(char* command_val, char* content,int msg_len);
void do_cd(char* path);
void ctrl_c_handler();
void child_killed();
/* ~function declarations */

/* Global declarations */

struct configurations
{
  int VSIZE;
  int HSIZE;
  int VSIZE_found;
  int HSIZE_found;
}config;

int bg_job;
char **command;

/* ~Global declarations */


/* Program section */

int main()
{

	//Handle CTRL+C signal
	signal(SIGINT,ctrl_c_handler); 
	signal(SIGCHLD,child_killed);
	

	//Check and load shconfig
	fetch_config();


	int i,inp_len;
	char msg[255];
	
	pid_t child_pid;
	int stat_loc;
	int loop_cntrl = 1;
	
	
	//Main loop control
	while(loop_cntrl != 0)
	{
		bg_job = 0;
		write(0,">",1);
		inp_len = readline(0,msg,255);

		//if input is NULL i.e its just a simple ENTER then continue
		if(inp_len==0)
		{	
			write(2,"\n",1);
			continue;
		}

		//If "&" job set background job to true
		if(msg[0]=='&'&&msg[1]==' ')
		{
			bg_job=1;
			memset(msg,' ',2);
		}
   
		//If one space input continue
		if(strcmp(msg," ")==0)
		{continue;}

		//!!!tokenize the input command!!!
		command = tokenize(msg);

		//!INTERNAL COMMAND "exit" handler
		loop_cntrl = strcmp(command[0],"exit");
		if(loop_cntrl==0)
		{
			break;
		}

		//!INTERNAL COMMAND "more" handler
		if(strcmp(command[0],"more")==0)
		{
			if(command[1]==NULL)
			{
				char* err_msg = malloc(80);
				err_msg="Please enter a file name to use the command 'more'. \n ex: more sample_file.txt \n";
				write(2,err_msg,80);
				err_msg = malloc(0);
				continue;
			}
			do_more(command[1]);
			continue;
		}

		// "clear" command handler . Clears the screen
		if(strcmp(command[0],"clear")==0)
		{
  			const char *clrscr = "\e[1;1H\e[2J";
  			write(1, clrscr, 11);
			continue;
		}

		//If the command is "cd" it cannot be perfectly done since the child process changes the directory, while the parent waits in the root
		//Handle "cd" explicitly		
		if(strcmp(command[0],"cd")==0)
		{
			do_cd(command[1]);
			continue;
		}


		
		//Fork child process if not background job
		if(bg_job!=1)
		{
			child_pid = fork();
		}

		//COMMAND EXECUTION 
		if(child_pid==0 && bg_job!=1)
		{

				int exec_ret =  execvp(command[0],command);
				if(exec_ret<0)
				{
					print_err_msg(command[0]," is an invalid command",80);					
					exit(0);
				}
				if(exec_ret>0)
				{
					exit(errno);
				}

		}
		else
		{
			if(bg_job!=1)
			{
				waitpid(child_pid, &stat_loc,WUNTRACED);  
				if(stat_loc!=0)
				{
				fprintf(stderr,"\nError in command execution %s\n",strerror(stat_loc));
				}
			}
			else
			{
				//Fork a child for background job and dont wait for exec return.
				pid_t first_child_pid=fork();
				if(first_child_pid==0)
				{
 
						int grand_exec_ret = execvp(command[0],command);
						if(grand_exec_ret < 0)
						{
						   print_err_msg(command[0]," is an invalid command (& process) \n>",80);			
							
						}	
						exit(0);

				}
				else
				{ 
					continue;
				}
				
			}
 
		}


		free(command);

	}


	return 0;
}

int readline(int fd, char* buf_str, size_t max)
{
	//This functions helps to read user input from the terminal	
	size_t i;
	for (i = 0; i < max; i++)
	{
		char tmp;
		int what = read(fd,&tmp,1);

		if (what == 0 || tmp == '\n')
		{
			buf_str[i] = '\0';
			return i;
		}
		buf_str[i] = tmp;
	}

	buf_str[i] = '\0';
	return i;
}

char** tokenize(char* input)
{
	//This function parses the input command into an array of strings
	char **command = malloc(8 * sizeof(char *));
	char *delimiter = " ";
	char *tokens;
	int index=0;
	tokens = strtok(input,delimiter);
	while(tokens!=NULL)
	{
		command[index] = tokens;
		index++;
		tokens=strtok(NULL,delimiter);
	}
	command[index]=NULL;
	return command;
}

void fetch_config()
{
	//This command checks for shconfig.config and fetch congifuration variables
	//It also handles if shconfig is not created
	int fp; 
	int i=0;
	int ret_read; 
	char buff[255];
	char **broke_line;
	int VSIZE = 0;
	fp = open("shconfig.config",O_RDONLY,S_IREAD);

	if(fp>=0)
	{

		while(read(fp,&buff,255))
		{

			if(strstr(buff,"VSIZE") != NULL || strstr(buff,"HSIZE") != NULL )
			{
				broke_line=tokenize(buff);

				if(strcmp(broke_line[0],"VSIZE")==0 && broke_line[2]==NULL)
				{
					config.VSIZE=atoi(broke_line[1]);
					config.VSIZE_found=1;
				}

				if(strcmp(broke_line[0],"HSIZE")==0 && broke_line[2]==NULL)
				{
					config.HSIZE=atoi(broke_line[1]);
					config.HSIZE_found=1;
				}
			}

			i++;

		}
		close(fp);
	}
	else
	{
		fp=open("shconfig.config",O_WRONLY | O_CREAT,S_IWRITE | S_IREAD);

		write(fp,"VSIZE 40\n",sizeof("VSIZE 40\n"));
		config.VSIZE_found=1;
		write(fp,"HSIZE 75\n",sizeof("HSIZE 75\n"));
		config.HSIZE_found=1;

		close(fp);
	}

	open("shconfig.config",O_WRONLY | O_CREAT,S_IWRITE | S_IREAD);
	if(config.VSIZE_found==0)
	{
		write(fp,"VSIZE 40\n",sizeof("VSIZE 40\n"));
		config.VSIZE=40;
		config.VSIZE_found=1;
	}
	if(config.HSIZE_found==0)
	{
		write(fp,"HSIZE 75\n",sizeof("HSIZE 75\n"));
		config.HSIZE=75;
		config.HSIZE_found=1;
	}
	close(fp);

}

void do_more(char* file_name)
{

	//This function handles the INTERNAL COMMAND "more"
	int rfp = open(file_name,O_RDONLY,S_IREAD);

	if(rfp<0)
	{
		char* err_msg = malloc(255);


		strcat(err_msg,file_name);
		strcat(err_msg," not found. \n");
		
		write(2,err_msg,255);
		

		return;
	}

	char line[config.HSIZE];
	int more_control=1;
	int inp_len;
	int read_ret;
	char msg[255]=" ";
	char **command;
	

	do 
	{       
		read_ret =0;
		if(strcmp(msg," ")==0)
	   	{


		   for(int l=0;l<=config.VSIZE;l++)
		   {

			read_ret = read(rfp,line,config.HSIZE);


			if(read_ret>0)
			{

				write(1,"\n",1);				
				write(1,line,(size_t)config.HSIZE);
				
			}
			else
			{
				more_control=0;
				break;
			}

			if(read_ret<=0)
			{
				more_control=0;
				break;
			}
		   }
		}
		else
		{
			more_control=0;		
			break;
		}
		inp_len = readline(0,msg,255);

	}while(more_control!=0);

	close(rfp);

}

void print_err_msg(char* command_val, char* content, int msg_len)
{
	char* err_msg = malloc(msg_len);
	strcat(err_msg,command_val);
	strcat(err_msg,content);
	write(2,err_msg,msg_len);
	err_msg = malloc(0);

}

void do_cd(char* path)
{
	int cd_ret = chdir(path);
	if(cd_ret!=0)
	{
		write(2,strerror(errno),strlen(strerror(errno)));
		write(2,"\n",1);
	}
	return;
}

void ctrl_c_handler()
{
	write(2,"\n>",2);
	return;
}



void child_killed()
{
	
		if(strcpy(command[0],"clear")!=0)
		{		
		write(2,"\n..Process Execution completed..\n>",34);
		}
		
}
/* ~Program section */

