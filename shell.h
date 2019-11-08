#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

// for server
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CMD_SIZE 100
#define MAX_ARGS_SIZE 100
#define MAX_USER_NAME 100
#define MAX_PATH_SIZE 1000
#define SHELL_INDICATOR "$ "
#define EXIT_CMD 31

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
	char HOME_DIR[MAX_PATH_SIZE];
	char curr_path[MAX_PATH_SIZE];
	char prev_path[MAX_PATH_SIZE];

	// user info
	char name[MAX_USER_NAME];
} S_User;

void removeWhiteSpace(char *cmd, int n);

int parseCommand(char *cmd, char **args)
{

	// TODO split command based on pipes first then split each individual part based on spaces
	int n = strlen(cmd);

	removeWhiteSpace(cmd, n);

	if (strcasecmp(cmd, "exit") == 0)
		return -1;

	// seperate command based on spaces => String.split(" ");
	char *ptr = strtok(cmd, " ");
	int j = 0;

	while (ptr != NULL)
	{
		args[j++] = ptr;
		ptr = strtok(NULL, " ");
	}

	// make last argument null string to signify end of array
	args[j] = NULL;
	return 0;
}

void setUpPaths(S_User *user, char *oldName)
{
	// getcwd(HOME, MAX_PATH_SIZE);

	strcpy(user->HOME_DIR, user->name);
	strtok(user->HOME_DIR, ": ");
	strcat(user->HOME_DIR, "/home");

	//first time changing
	if (oldName == NULL)
	{
		strcpy(user->curr_path, user->HOME_DIR);
		strcpy(user->prev_path, "");
	}
	else
	{
		char temp[MAX_PATH_SIZE];
		char newPath[MAX_PATH_SIZE];

		// set up old name to copy it to remove it from the path and remove unnecessary parts
		char old_name_cpy[MAX_USER_NAME];
		strcpy(old_name_cpy, oldName);
		strtok(old_name_cpy, " :\n\r");

		//update current path
		// add curent user path to temp
		strcpy(temp, user->curr_path);
		// copy new username to newPath
		strcpy(newPath, user->name);

		int i = strlen(user->name);
		int j = 0;

		// skip the old name found in temp and then copy the rest of temp into newPath
		while (i < MAX_PATH_SIZE)
		{
			if (i > strlen(temp))
				break;
			while (j < strlen(old_name_cpy))
			{
				if (temp[i] == old_name_cpy[j])
					j++;
				else
					break;
			}
			newPath[i] = temp[i];
			i++;
		}
		strcpy(user->curr_path, newPath);

		//update previous path
		// add previous user path to temp
		strcpy(temp, user->prev_path);
		// copy new username to newPath
		strcpy(newPath, user->name);

		i = strlen(user->name);
		j = 0;

		// skip the old name found in temp and then copy the rest of temp into newPath
		while (i < MAX_PATH_SIZE)
		{
			if (i > strlen(temp))
				break;
			while (j < strlen(old_name_cpy))
			{
				if (temp[i] == old_name_cpy[j])
					j++;
				else
					break;
			}
			newPath[i] = temp[i];
			i++;
		}
		strcpy(user->prev_path, newPath);
	}
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

// TODO change pwd when changing username.
int assignUsername(S_User *user, char *newName)
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
		strcpy(oldName, user->name);
		strcpy(user->name, newName);
		setUpPaths(user, oldName);
	}

	if (strcmp(user->name, "") != 10 && strcmp(user->name, " ") != 32 && strcmp(user->name, " ") != -22)
	{
		strtok(user->name, " \n\r");
		strcat(user->name, ": ");
		return 0;
	}
	return 1;
}
// int assignUsername(char name[], int n, char *newName)
// {
// 	// getting username from user for personalized experience

// 	if (newName == NULL)
// 	{
// 		printf("please enter a new username: ");
// 		fgets(name, MAX_USER_NAME, stdin);
// 	}
// 	else
// 		strcpy(name, newName);

// 	if (strcmp(name, "") != 10 && strcmp(name, " ") != 32 && strcmp(name, " ") != -22)
// 	{
// 		strtok(name, " \n\r");
// 		strcat(name, ": ");
// 		return 0;
// 	}
// 	return 1;
// }

void removeWhiteSpace(char *cmd, int n)
{
	// removing preceeding whitespace
	for (int i = 0; i < n; i++)
	{
		if (isspace(cmd[i]))
		{
			for (int j = i; j < n; j++)
			{
				int temp = cmd[j];
				cmd[j] = cmd[j + 1];
				cmd[j + 1] = temp;
			}
		}
		else
			break;
	}

	// removing trailing whitespace
	for (int i = n - 1; i >= 0; i--)
	{
		if (isspace(cmd[i]))
			cmd[i] = '\0';
		else
			break;
	}

	// redundant check to be sure
	strtok(cmd, "\n\r");
}

int hasPipes(char *cmd)
{
	int n = strlen(cmd);
	for (int i = 0; i < n; i++)
		if (cmd[i] == '<' || cmd[i] == '>' || cmd[i] == '|')
			return i;
	return -1;
}
