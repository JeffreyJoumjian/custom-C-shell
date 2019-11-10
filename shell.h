
#include "helper.h"

// for server
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CMD_SIZE 100
#define MAX_ARGS_SIZE 100
#define MAX_USER_NAME 100
#define MAX_PATH_SIZE 1000
#define SHELL_INDICATOR "$ "
#define EXIT_CMD 31
#define READ_END 0
#define WRITE_END 1

// this is used later on to know if we have a custom command (cleaner code purposes)
const char *CUSTOM_CMDS[] = {"exit", "user -n", "cd", "pwd", "help"};
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
	char prev_path[MAX_PATH_SIZE];

	// user info
	char name[MAX_USER_NAME];
} S_User;

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

	// seperate command based on spaces => String.split(" ");
	char *ptr = String_remove(cmd, " ");
	int j = 0;

	while (ptr != NULL)
	{
		args[j++] = ptr;
		ptr = String_remove(NULL, " ");
	}

	// make last argument null string to signify end of array
	args[j] = NULL;

	return cmd_type; // return true
}

void setUpPaths(S_User *user)
{

	// DEF changing user name from empty user to named user is causing problems
	// FIX pwd no longer shows username to avoid this issue
	// String_copy(user->HOME_DIR, user->name);
	// String_remove(user->HOME_DIR, ": \n");
	// String_concat(user->HOME_DIR, "/home");

	String_copy(user->HOME_DIR, "/home");

	//first time changing that is current path is still not set up
	if (user->curr_path[0] == '\0')
	{
		String_copy(user->curr_path, user->HOME_DIR);
		String_copy(user->prev_path, "");
	}
	else
	{

		char temp[MAX_PATH_SIZE];
		char newPath[MAX_PATH_SIZE];

		// if (oldName[0] == '\n')
		// {

		// update current path
		// String_copy(newPath, user->name);
		String_concat(newPath, user->curr_path);
		String_remove(newPath, ": ");
		String_copy(user->curr_path, newPath);

		// update previous path
		// String_copy(newPath, user->name);
		String_concat(newPath, user->prev_path);
		String_remove(newPath, ": ");
		String_copy(user->prev_path, newPath);

		// }

		// else
		// {

		// 	// set up old name to copy it to remove it from the path and remove unnecessary parts
		// 	char old_name_cpy[MAX_USER_NAME];
		// 	String_copy(old_name_cpy, oldName);
		// 	String_remove(old_name_cpy, " :\n\r");

		// 	//update current path
		// 	// add curent user path to temp
		// 	String_copy(temp, user->curr_path);
		// 	// copy new username to newPath
		// 	String_copy(newPath, user->name);

		// 	int i = strlen(user->name);
		// 	int j = 0;

		// 	// skip the old name found in temp and then copy the rest of temp into newPath
		// 	while (i < MAX_PATH_SIZE)
		// 	{
		// 		if (i > strlen(temp))
		// 			break;
		// 		while (j < strlen(old_name_cpy))
		// 		{
		// 			if (temp[i] == old_name_cpy[j])
		// 				j++;
		// 			else
		// 				break;
		// 		}
		// 		newPath[i] = temp[i];
		// 		i++;
		// 	}
		// 	String_copy(user->curr_path, newPath);

		// 	//update previous path
		// 	// add previous user path to temp
		// 	String_copy(temp, user->prev_path);
		// 	// copy new username to newPath
		// 	String_copy(newPath, user->name);

		// 	i = strlen(user->name);
		// 	j = 0;

		// 	// skip the old name found in temp and then copy the rest of temp into newPath
		// 	while (i < MAX_PATH_SIZE)
		// 	{
		// 		if (i > strlen(temp))
		// 			break;
		// 		while (j < strlen(old_name_cpy))
		// 		{
		// 			if (temp[i] == old_name_cpy[j])
		// 				j++;
		// 			else
		// 				break;
		// 		}
		// 		newPath[i] = temp[i];
		// 		i++;
		// 	}
		// 	String_copy(user->prev_path, newPath);
		// }
	}
}

int isHomeDir(S_User *user)
{
	return strcasecmp(user->curr_path, user->HOME_DIR) == 0;
}

void pathBack(S_User *user, char *req_path)
{
	char *paths[MAX_PATH_SIZE];

	char *ptr = String_remove(req_path, "/");
	int j = 0;

	while (ptr != NULL)
	{
		paths[j++] = ptr;
		ptr = String_remove(NULL, "/");
	}

	paths[j] = NULL;

	for (int i = 0; i < j; i++)
	{
		// can't go up if already in user directory
		if (strcmp(paths[i], "..") == 0 && isHomeDir(user))
		{
			perror("Cannot go further than home directory");
			return;
		}
		if (chdir(paths[i]) < 0)
			perror(NULL);
		else
			String_copy(user->curr_path, user->prev_path);
	}
}

void pathForward(S_User *user, char *next)
{
	// assign current path to previous
	// update current path to next
	// change to next directory
	String_copy(user->prev_path, user->curr_path);
	String_concat(user->curr_path, "/");
	String_concat(user->curr_path, next);
	if (chdir(next) < 0)
		perror(NULL);
}

// TODO change pwd when changing username.
void assignUsername(S_User *user, char *newName)
{
	// getting username from user for personalized experience
	if (newName == NULL)
	{
		printf("please enter a new username: ");
		fgets(user->name, MAX_USER_NAME, stdin);
	}
	else
	{
		char oldName[MAX_USER_NAME];
		String_copy(oldName, user->name);
		String_copy(user->name, newName);
		// setUpPaths(user);
	}

	// if (strcmp(user->name, "") != 10 && strcmp(user->name, " ") != 32 && strcmp(user->name, " ") != -22)
	// {
	String_remove(user->name, " \n\r");
	if (user->name[0] != '\n')
		String_concat(user->name, ": ");
	// }
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
		"5 - user -n <new_username> (changes the username of the current user)\n"
		"6 - ls, ps, rm ... (supports all UNIX commands with their arguments)\n");
	puts("\n*** Jeffrey Joumjian - Maria Kantardjian - Reem Saado (alphabetically) ***\n");
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

	if (strcasecmp(args[0], "exit") == 0)
		exit(0);

	else if (strcasecmp(args[0], "cd") == 0)
	{
		// if cd doesn't have arguments
		if (args[1] == NULL)
			printf("empty");
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
	else if (strcasecmp(args[0], "pwd") == 0)
		printf("%s\n", USER->curr_path);

	// if cmd == user -n => change username
	else if (strcasecmp(args[0], "user") == 0 && strcasecmp(args[1], "-n") == 0)
		assignUsername(USER, args[2]);

	// print working directory
	else if (strcasecmp(args[0], "pwd") == 0)
		printf("%s\n", USER->curr_path);

	// if cmd == user -n => change username
	else if (strcasecmp(args[0], "user") == 0 && strcasecmp(args[1], "-n") == 0)
		assignUsername(USER, args[2]);

	else if (strcasecmp(args[0], "help") == 0)
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
		execvp(args[0], args);

	else if (pid > 0)
	{
		pid = wait(0);
		printf("\n");
	}
}