#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>

#define CMD_ARRAY_SIZE 11
#define MAX_CMD_SIZE 100
#define MAX_ARGS_SIZE 100
#define MAX_USER_NAME 100
#define MAX_PATH_SIZE 1000
#define SHELL_INDICATOR "$ "

typedef struct user
{
	// paths
	char HOME_DIR[MAX_PATH_SIZE];
	char curr_path[MAX_PATH_SIZE];
	char prev_path[MAX_PATH_SIZE];

	// user info
	char name[MAX_USER_NAME];
} S_User;

int parseCommand(char *cmd, char **args)
{

	// removing trailing whitespace
	for (int i = strlen(cmd) - 1; i >= 0; i--)
	{
		if (isspace(cmd[i]))
			cmd[i] = '\0';
		else
			break;
	}

	// if cmd == exit => stop parsing
	int n = strlen(cmd);
	if (n == 4)
	{
		char *exit = "exit";
		for (int i = 0; i < n; i++)
		{
			if (tolower(cmd[i]) != exit[i])
				break;
			return -1;
		}
	}

	char *ptr = strtok(cmd, " ");
	int j = 0;

	while (ptr != NULL)
	{
		args[j++] = ptr;
		ptr = strtok(NULL, " ");
	}

	args[j] = NULL;
	return 0;
}

// void setUpPaths(char *curr, char *HOME, char *last)
// {
// 	// getcwd(HOME, MAX_PATH_SIZE);
// 	strcpy(HOME, "/home");
// 	strcpy(curr, HOME);
// 	strcpy(last, "");
// }
void setUpPaths(S_User *user)
{
	// getcwd(HOME, MAX_PATH_SIZE);
	strcpy(user->HOME_DIR, user->name);
	strtok(user->HOME_DIR, ": ");
	strcat(user->HOME_DIR, "/home");
	strcpy(user->curr_path, user->HOME_DIR);
	strcpy(user->prev_path, "");
}

int isHomeDir(S_User *user)
{
	return strcasecmp(user->curr_path, user->HOME_DIR);
}

void pathBack(S_User *user, char *req_path)
{
	char *paths[MAX_PATH_SIZE];

	char *ptr = strtok(req_path, "/");
	int j = 0;

	while (ptr != NULL)
	{
		paths[j++] = ptr;
		ptr = strtok(NULL, "/");
	}

	paths[j] = NULL;

	for (int i = 0; i < j; i++)
	{
		// can't go up if already in user directory
		if (strcmp(paths[i], "..") == 0 && isHomeDir(user) == 0)
		{
			perror("Cannot go further than home directory");
			return;
		}
		chdir(paths[i]);
		strcpy(user->curr_path, user->prev_path);
	}
}

void pathForward(S_User *user, char *next)
{
	// assign current path to previous
	// update current path to next
	// change to next directory
	strcpy(user->prev_path, user->curr_path);
	strcat(user->curr_path, "/");
	strcat(user->curr_path, next);
	if (chdir(next) < 0)
		perror(NULL);
}

int assignUsername(char name[], int n, char *newName)
{
	// getting username from user for personalized experience

	if (newName == NULL)
	{
		printf("please enter a new username: ");
		fgets(name, MAX_USER_NAME, stdin);
	}
	else
		strcpy(name, newName);

	if (strcmp(name, "") != 10 && strcmp(name, " ") != 32 && strcmp(name, " ") != -22)
	{
		strtok(name, " \n\r");
		strcat(name, ": ");
		return 0;
	}
	return 1;
}
