About the project:
------------------

This project is a custom implementation of bash using C and file manipulation with the POSIX API.
The custom shell executes all commands which are native to the inbuilt UNIX shell with the help of execvp() call. 
It also includes two further internal functionalities "more" and "exit". The command "more" is used to browse through a text file and "exit" will terminate the shell. 

How to run:
-----------

1. run the command "make clean & make" in the project root directory after cloning it.
2. run "./sshell" to run the custom shell. I call it CBASH ;)


Program Explanation:
--------------------

The program has 9 functions,

int main();
int readline(int fd, char* buf_str, size_t max);//function by : Dr. Stefan Bruda
char** tokenize(char* input);//function organised from web reference and strtok() man pages
void fetch_config();
void do_more(char* file_name);
void print_err_msg(char* command_val, char* content,int msg_len);
void do_cd(char* path);
void ctrl_c_handler();
void child_killed();

int main()
----------

The main function of the program which controls the flow of the process. On successfull invocation of the "sshell" application, this function is started.

This function whenever called first handles all the "signals" prompted to it. 
It invokes the function "void ctrl_c_handler()" to handle the SIGINT interupption from the user or terminal.
It also handles the SIGCHLD signal from child process by calling the "void child_killed()" function.


Upon ensuring that signals have been handled, the first and important thing the function fetches is the configurations from the "shconfig.config" file.
The configuration fetch job is being handed over to the "void fetch_config();" function.
	
void fetch_config();
--------------------

This function reads the "shconfig.config" file from the parent directory for the VSIZE and HSIZE configuration values.
It makes sure that it reads the following two lines only,

VSIZE v
HSIZE h

It does not read any other line than the above specified ones.	

Incase of absence of any one of the following two or the absence of the above two lines upon file existence, the missing configuration are 
written to the file with the value of v as 40 and h as 75.

If the file "shconfig.config" does not exists, the program creates it and writes the above two configuration.
The configurtions are fetched and loaded into the "configurations" struct. These data will be used by the command "more" if invoked.

Once after successful configuration fetch the variables are initialized.

After which, the main control loop begins. This loop enables the functionality of prompting the user for the next input on successfull completion of one.

The main Loop
-------------

The function "int readline(int fd, char* buf_str, size_t max);" is invoked and input from the user terminal is fetched into the variable "msg".
The function loads the input into the variable "msg" and returns the length of the input.

If the input length is 0, it is assumed that no command but a simple ENTER was issued, so the terminal breaks a line prompts for input once again.
The same process is done when a single space is issued as input(Future Developments planned).

Following the input from user terminal it is then tokenized by the function "char** tokenize(char* input);" and returned into the global varibale "command".

The function "char** tokenize(char* input);" used the strtok(3) function to achieve its functionality.
The input string is broke with " " being considered as the delimiter.

Once we obtain the tokenized command we then proceed to its execution.

If the command is "exit", then a loop control variable is set to "false" so that the loop fails on condition and is issued a "break;" so that the application terminates once out of the loop.

If the command is "more", the execution flow is then handed to the "void do_more(char* file_name);" function.
	
	void do_more(char* file_name);
	------------------------------
	
	This function opens the file which was passed to it as an arrgument from the command. If the file is not found it prompts an error and seeks 		further user terminal input.

	If found it then reads the file "h" characters per time and prints it to screen. "v" such lines are printed at a time.

	Upon printing "v" lines the command seeks user input. 

	If user terminal input is a " " then the next batch of "v" lines are printed.

	The command is completed on EOF of the file.
	The command is terminated on any other input rather than " ".


Further the loop checks if the command is "clear". If clear the ANSI escape character for terminal screen clear is printed the stdout.
(The default clear command was not considered because it will be executed in a child process throwing SIGCHLD upon compltion and not clearing the screen as expected.)

Next if the command is "cd" is checked for and is handled by the "void do_cd()" function which just makes use of the chdir() library function to achieve it functionality. The "cd" command was not working in the execvp() call since the command will be executed in the child process and not allowing the environment to be changed since the parent process is responsible for that.

FINALLY any other command than the above are handled. The command are called the external command.

The external commands are not executed in the same parent process because the execvp() call takes over the parent process id and wont return control.

In order to avoid that a new child process is fork()ed and the commands are passed to execvp() in it.
	If the command is not found "$command not found" message is prompted.
	If in case of any execution error, the child process exits with the "errno" of the error.
	If in case of successful exection, execvp() doesnt return control and the child process is considered "Successfull Execution".

	The parent process in the mean time, once on child fork(), waits for the child process to complete.
	The waitpid() function helps achieve the waiting functionality.
	The parent process on completion of the child process claims two input from the kernel.
	1. The SIGCHLD signal, the parent process displays a message based on this signal.
	   This signal is handled by the function "void child_killed();" which just prints a message.
	2. "child return status". The command execution error is judged and printed based on this in case of any error to the executed command.
	    A message is thrown by the shell indicating the error in the command.


	The "&" commands.
	-----------------
	In case of a "&" i.e "Background command" the parent process fork()s a child process and does not waits for it to complete.
	However the child process handles the execvp() and prompts in case of any error.
	It will anyway send a SIGCHLD to the parent. The parent (which however will be or if active) receives the SIGCHLD and indicates the user 		terminal of its completion.
	The error from the background process are not specifically denoted as foreground since the retun status of the background process is not 		monitored.

Atlast,
Whenver and wherever a message is to be prompted to the user the function "void print_err_msg(char* command_val, char* content,int msg_len);" is invoked. It writes the message to "stderr" to communicate to the user terminal.

TEST SUITE
----------

Test 1: CASE 	- shconfig.config file is deleted and program is invoked.
	OUTPUT 	- program executes successfully, creates the file and populates it.

Test 2: CASE 	- unnecessary lines are typed in the "shconfig.config" file.
	OUTPUT	- program ignores other lines and considers the two required lines only

Test 3: CASE 	- alternative lines from the two requrired config lines are deleted from the "shconfig.config".
	OTUPUT 	- application executes successfully and append the missing line to the file.

Test 4: CASE 	- To check if application executes all inbuilt UNIX commands.
	OUTPUT 	- application successfully executes the inbuilt commands. Displays a message on successfull completion or any relatable error message.

Test 5: CASE 	- To check if ENTER or " " breaks line and reprompts for input.
	OUTPUT 	- application breaks line and reprompts user input.

Test 6: CASE 	- Execution of the command more.
	OUTPUT 	- program displays the file specified and waits for user interuppt. if user inputs " " it reads further.
		 On any other input or EOF, "more" terminates and the shell reprompts.

Test 7: CASE 	- Execution of "&"-"background proces".
	OUTPUT  - Application triggers the command and prompts for next input.
		    Application denotes a successfull "completion message" once the background process both completes or fail.
	TOOl 	- A C program which waits for 3 seconds once triggered.

Test 8: CASE 	- Execution of the command "more".
	OUTPUT 	- Application terminates.

REFERENCES:
-----------

1. Man Pages of all the API functions used.
2. "Advanced Programming in the UNIX Environment" by W. Richard Stevens and Stephen A. Rago (research on the signals and concurrency processes). 
