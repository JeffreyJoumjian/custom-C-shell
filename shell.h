#include "helper.h"

// for server
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CMD_SIZE 100
#define MAX_ARGS_SIZE 100
#define MAX_USER_NAME 100
#define MAX_PATH_SIZE 1000
#define MAX_PIPE_SIZE 20
#define SHELL_INDICATOR "$ "
#define READ_END 0
#define WRITE_END 1

// this is used later on to know if we have a custom command (cleaner code purposes)
const char *CUSTOM_CMDS[] = {"exit", "user", "cd", "pwd", "help"};
const char *COLUMN_WIDTH[] = {"--cols", "100000000"};
const int CSTM_CMDS_SIZE = sizeof(CUSTOM_CMDS) / sizeof(char *);

// for server
#define MAX_LINE 4096
#define BACKLOG 10
#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"

// for client
#define CLIENT_GREET "you have reached the server, please send an input: \0"
#define CLIENT_RESPONSE "your response was: \0"

typedef struct client
{
	int id;
	char input[MAX_LINE];
	char res[MAX_LINE];
} CLIENT;

typedef struct user
{
	// paths
	const char SHELL_DIR[MAX_PATH_SIZE];
	char HOME_DIR[MAX_PATH_SIZE];
	char curr_path[MAX_PATH_SIZE];

	// user info
	char name[MAX_USER_NAME];
	time_t start_time;
} S_User;

// checks if the cmd is one of the commands we defined such as exit, cd, pwd ...
int isCustomCommand(char *cmd)
{
	for (int i = 0; i < CSTM_CMDS_SIZE; i++)
		if (String_isSubstring(cmd, CUSTOM_CMDS[i]))
			return 1;
	return 0;
}

int numPipes(char **args)
{

	int num_pipes = 0;
	for (int i = 0; i < MAX_ARGS_SIZE && args[i] != NULL; i++)
		if (String_Equals(args[i], "|"))
			num_pipes++;
	return num_pipes;
}
int isPipe(char c)
{
	if (c == '|')
		return 0;
	// if (c == '<')
	// 	return 1;
	// if (c == '>')
	// 	return 2;
	return -1;
}

// returns 0 if command if exit or built in command
// returns 1 if command doesn't have pipes
// returns 2 if command has pipes (|)
// char *cmd, char **args
int parseCommand(char *cmd, char *args[], char temp[])
{
	// TODO split command based on pipes first then split each individual part based on spaces
	int n = strlen(cmd);

	removeWhiteSpace(cmd, n);

	int cmd_type = 0;

	if (isCustomCommand(cmd))
		cmd_type = 0;
	else
		cmd_type = 1;

	// clear previous args
	clearStringArray(args, MAX_ARGS_SIZE);

	// clear temp
	for (int i = 0; i < MAX_CMD_SIZE; i++)
		temp[i] = '\0';

	int j = 0;

	for (int i = 0; i < n; i++)
	{
		if (isPipe(cmd[i]) >= 0)
		{
			if (i == 0 || i == n - 2)
			{
				perror("Incorrect use of pipes");
				return -1;
			}

			temp[j++] = ' ';
			temp[j++] = cmd[i];
			temp[j++] = ' ';

			cmd_type = 2;
		}

		else if ((isascii(cmd[i]) || cmd[i] == ' ') && (cmd[i] != '\n' || cmd[i] != '\r'))
			temp[j++] = cmd[i];
	}

	temp[j] = '\0';

	// // seperate command based on spaces => String.split(" ");
	char *ptr = String_splitFirst(temp, " ");
	j = 0;

	while (ptr != NULL)
	{

		args[j++] = ptr;
		ptr = String_splitFirst(NULL, " ");
	}

	// make last argument null string to signify end of array
	args[j] = NULL;

	return cmd_type; // return true
}

void updateCurrentPath(S_User *user)
{
	char temp[MAX_PATH_SIZE];

	// update current path
	getcwd(temp, MAX_PATH_SIZE);

	String_remove(temp, (char *)user->SHELL_DIR);
	String_copy(user->curr_path, temp);
}

void setUpPaths(S_User *user)
{

	getcwd((char *)user->SHELL_DIR, MAX_PATH_SIZE);

	if (chdir("home") < 0)
	{
		perror("User does not have home directory");
		mkdir("home", 0770);
		mkdir("home/desktop", 0770);
	}

	getcwd(user->HOME_DIR, MAX_PATH_SIZE);
	String_remove(user->HOME_DIR, (char *)user->SHELL_DIR);
	String_copy(user->curr_path, user->HOME_DIR);
}

int isHomeDir(S_User *user)
{
	return String_EqualsIgnoreCase(user->curr_path, user->HOME_DIR);
}

void pathBack(S_User *user, char *req_path)
{
	char *paths[MAX_PATH_SIZE];

	char *ptr = String_splitFirst(req_path, "/");
	int j = 0;

	while (ptr != NULL)
	{
		paths[j++] = ptr;
		ptr = String_splitFirst(NULL, "/");
	}

	paths[j] = NULL;

	for (int i = 0; i < j; i++)
	{
		// can't go up if already in user directory
		if (String_Equals(paths[i], "..") && isHomeDir(user))
		{
			perror("Cannot go further than home directory");
			return;
		}
		if (chdir(paths[i]) < 0)
			perror(NULL);
		else
			updateCurrentPath(user);
	}
}

void pathForward(S_User *user, char *next)
{
	if (chdir(next) < 0)
		perror(NULL);

	updateCurrentPath(user);
}

void assignUsername(S_User *user, char *newName)
{
	// getting username from user for personalized experience
	if (newName == NULL)
	{
		printf("please enter a new username> ");
		fgets(user->name, MAX_USER_NAME, stdin);
	}
	else
	{
		char oldName[MAX_USER_NAME];
		String_copy(oldName, user->name);
		String_copy(user->name, newName);
	}

	String_splitFirst(user->name, " \n\r");
	if (user->name[0] != '\n')
		String_concat(user->name, ": ");
}

void setUpUser(S_User *USER)
{
	time(&USER->start_time); // start user time
	assignUsername(USER, NULL);
	setUpPaths(USER);
}

void help()
{
	puts("\n*** WELCOME TO THE HELP GUIDE ***\n");
	puts("----------------------------------\n");

	puts("Supported Commands:\n");
	puts("-------------------\n");
	puts(
		"1 - help (prints the help manual for the shell)\n"
		"2 - exit (exits the shell)\n"
		"3 - cd <dst_path> (used to change the working directory)\n"
		"4 - pwd (prints the current working directory of the user <username/home/...>)\n"
		"5 - user [ -i, -n ]\n"
		"\t-n <new_username> (changes the username of the current user)\n"
		"\t-i (print user info)\n"
		"6 - ls, ps, rm ... (supports all UNIX commands with their arguments)\n"
		"7 - pipe support for up to 21 commands\n");
	puts("\n*** Jeffrey Joumjian - Maria Kantardjian - Reem Saado (alphabetically) ***\n");
}

void printUserInfo(S_User user)
{
	printf("user: %s\n", user.name);
	printf("home dir: %s\n", user.HOME_DIR);
	printf("current dir: %s\n", user.curr_path);

	time_t current_time;
	time(&current_time);
	printf("time elapsed: %.3fs\n", (double)difftime(current_time, user.start_time));
}

// get input from user and put it in the cmd string
int getInput(char *cmd)
{

	char buff[MAX_CMD_SIZE];
	if (fgets(buff, MAX_CMD_SIZE, stdin) > 0 && buff[0] != '\n')
	{
		String_copy(cmd, buff);
		return 1; // return true if successful scan
	}
	return 0; // return false if empty
}

void execCustomCommand(char *args[], S_User *USER)
{
	printf(YEL);

	if (String_EqualsIgnoreCase(args[0], "exit"))
		exit(0);

	else if (String_EqualsIgnoreCase(args[0], "cd"))
	{
		// if cd doesn't have arguments
		if (args[1] == NULL)
			printf("");
		else
		{
			// change directory and update curr_path
			if (strstr(args[1], "..") != NULL)
				pathBack(USER, args[1]);
			else
				pathForward(USER, args[1]);
		}
	}

	// 	// print working directory
	else if (String_EqualsIgnoreCase(args[0], "pwd"))
		printf("%s\n\n", USER->curr_path);

	// if cmd == user -n => change username
	else if (String_EqualsIgnoreCase(args[0], "user"))
	{
		if (args[1] == NULL)
			printf("%s is not a valid argument. user [ -i, -n ]\n", args[1]);
		else if (String_EqualsIgnoreCase(args[1], "-i"))
			printUserInfo(*USER);
		else if (String_EqualsIgnoreCase(args[1], "-n"))
			assignUsername(USER, args[2]);
		else
			printf("%s is not a valid argument. user [ -i, -n ]\n", args[1]);
	}

	else if (String_EqualsIgnoreCase(args[0], "help"))
		help();

	printf(RESET);
}

void execCommand(char *args[])
{

	// create child to run command
	pid_t pid = fork();

	if (pid < 0)
	{
		perror("failed to create child.\n");
		exit(1);
	}

	else if (pid == 0)
	{
		if (execvp(args[0], args) < 0)
		{
			perror("Command not found\n");
			exit(1);
		}
	}

	else if (pid > 0)
	{
		pid = wait(0);
		printf("\n");
	}
}

int getNextPipe(char *args[], int start)
{
	int i;
	for (i = start; i < MAX_ARGS_SIZE && args[i] != NULL; i++)
		if (String_Equals(args[i], "|"))
			return i;
	return i;
}

// takes start and end indices and copies the commands between start & end from args to piped_args
void getArgsForCurrentExec(char *args[], char *piped_args[], int start, int end)
{
	// clear previous piped args
	clearStringArray(piped_args, MAX_PIPE_SIZE);

	int j = 0;
	for (int i = start; i < end || args[i + 1] == NULL; i++)
		piped_args[j++] = args[i];

	piped_args[j] = NULL;
}

void execPipedCommand(char *args[], char *piped_args[], char temp[], S_User *user)
{

	/* 
	start & end indices to get the section of commands from args to execute
	no need for shared memory because at every pipe the values are being updated in the parent
	and the new forked child is spawning with this new updated values.
	*/
	int start = 0;
	int end = getNextPipe(args, start);

	int fd[2];
	int fdd = 0;

	int num_cmds = numPipes(args) + 1;

	// for every command in args
	for (int i = 0; i < num_cmds; i++)
	{
		pipe(fd);

		pid_t pid = fork(); // fork child to execute command

		if (pid == 0)
		{
			getArgsForCurrentExec(args, piped_args, start, end); // get the cmds between args[start] -> args[end] to execute them

			close(fd[READ_END]);
			dup2(fdd, READ_END); // get input from pipe instead of stdin
			if (i + 1 != num_cmds)
			{
				dup2(fd[WRITE_END], WRITE_END); // write output to pipe instead of stdout
				dup2(fd[WRITE_END], 2);			// write error to pipe instead of stdout
			}
			close(fd[WRITE_END]);

			// we must first check if it's a custom command
			if (isCustomCommand(piped_args[0]))
			{
				execCustomCommand(args, user);
				exit(0);
			}
			else
			{
				// this is to make sure that ps buffer output correctly
				if (String_EqualsIgnoreCase(piped_args[0], "ps") && piped_args[1] != NULL && String_EqualsIgnoreCase(piped_args[1], "aux"))
				{
					for (int j = 2; j < MAX_ARGS_SIZE; j++)
					{
						if (piped_args[j] == NULL)
						{
							piped_args[j] = "--cols";
							piped_args[j + 1] = "1000000000";
							piped_args[j + 2] = NULL;
							break;
						}
					}
				}
				// check if it has redirection => if so change the dups to input/output from file (to be implemented for phase 2)

				// else exec normal command
				if (execvp(piped_args[0], piped_args) < 0)
				{
					printf("error while executing command %d", i);
					exit(1);
				}
			}
		}
		if (pid > 0)
		{
			waitpid(pid, NULL, 0);
			close(fd[WRITE_END]);
			fdd = fd[READ_END];

			start = end + 1;
			end = getNextPipe(args, start);
		}
		if (pid < 0)
		{
			perror("failed to create child.\n");
			exit(1);
		}
	}
	close(fd[READ_END]);
	close(fd[WRITE_END]);
}

void execRedirectedCommand(char *args[], char *red_args[])
{
	// for()
}