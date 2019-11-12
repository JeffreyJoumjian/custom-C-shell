
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

typedef struct cmd
{
	char cmd[MAX_CMD_SIZE];
	char *args[MAX_ARGS_SIZE];
	int pipe_locs[MAX_PIPE_SIZE];
	char pipe_types[MAX_PIPE_SIZE];
} COMMAND;

// checks if the cmd is one of the commands we defined such as exit, cd, pwd ...
int isCustomCommand(char *cmd)
{
	for (int i = 0; i < CSTM_CMDS_SIZE; i++)
		if (String_isSubstring(cmd, CUSTOM_CMDS[i]))
			return 1;
	return 0;
}

int hasPipes(char *cmd)
{
	int n = strlen(cmd);
	for (int i = 0; i < n; i++)
		if (cmd[i] == '<' || cmd[i] == '>' || cmd[i] == '|')
			return i;
	return -1;
}

// returns 0 if command if exit or built in command
// returns 1 if command doesn't have pipes
// returns 2 if command has redirection (<,>,<<,>>) or pipes (|)
int parseCommand(char *cmd, char **args)
{

	// TODO split command based on pipes first then split each individual part based on spaces
	int n = strlen(cmd);

	removeWhiteSpace(cmd, n);

	int cmd_type = 0;

	if (isCustomCommand(cmd))
		cmd_type = 0;
	else if (hasPipes(cmd) >= 0)
		cmd_type = 2;
	else
		cmd_type = 1;

	// clear previous args
	for (int i = 0; i < MAX_ARGS_SIZE; i++)
	{
		if (args[i] == NULL)
			break;
		args[i] = NULL;
	}

	// seperate command based on spaces => String.split(" ");
	char *ptr = String_splitFirst(cmd, " ");
	int j = 0;

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
		exit(1);
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
		"7 - pipe support coming soon\n");
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
		printf("%s\n", USER->curr_path);

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