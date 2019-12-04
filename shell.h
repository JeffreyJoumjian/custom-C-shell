#include "helper.h"

// for server
#include <pthread.h>
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
const char *CUSTOM_CMDS[] = {"exit", "user", "cd", "pwd", "help", "mkdir", "rmdir", "rm -r", "rm"};
const int CSTM_CMDS_SIZE = sizeof(CUSTOM_CMDS) / sizeof(char *);

// for server
#define MAX_LINE 4096
#define BACKLOG 10
#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"

// for client
char get_username[] = "please enter a new username> ";

typedef struct
{
	// paths
	char SHELL_DIR[MAX_PATH_SIZE];
	char HOME_DIR[MAX_PATH_SIZE];
	char curr_path[MAX_PATH_SIZE];

	// user info
	char name[MAX_USER_NAME];
	time_t start_time;

} S_User;

typedef struct
{
	int id;
	char cmd[MAX_CMD_SIZE];
	char temp[MAX_LINE];
	char *args[MAX_ARGS_SIZE];
	char *piped_args[MAX_ARGS_SIZE];

	S_User user;
	int socket;

} CLIENT;

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
	return -1;
}

// returns 0 if command if exit or built in command
// returns 1 if command doesn't have pipes
// returns 2 if command has pipes (|)
int parseCommand(char *cmd, char *args[], char temp[])
{
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

	// go over the string and insert the appropriate spaces to split the command based on spaces later
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

	// seperate command based on spaces => String.split(" ");
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

void printShell(CLIENT *client)
{
	printf(RED "%s", client->user.name);
	printf(RESET);
	printf(YEL "~%s ", client->user.curr_path);
	printf(RESET);
	printf(GRN "%s", SHELL_INDICATOR RESET);
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
	chdir("/Users/mbp/Desktop/os-project");

	getcwd((char *)user->SHELL_DIR, MAX_PATH_SIZE);

	if (chdir("home") < 0)
	{
		perror("User does not have home directory");
		// mkdir("home", 0770);
		// mkdir("home/desktop", 0770);
	}

	getcwd(user->HOME_DIR, MAX_PATH_SIZE);
	String_remove(user->HOME_DIR, (char *)user->SHELL_DIR);
	String_copy(user->curr_path, user->HOME_DIR);
}

int isHomeDir(S_User *user)
{
	return String_EqualsIgnoreCase(user->curr_path, user->HOME_DIR);
}

void pathBack(S_User *user, char *req_path, char *res)
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
		// can't go up if already in user's home directory
		if (String_Equals(paths[i], "..") && isHomeDir(user))
		{
			snprintf(res, MAX_LINE, "Cannot go further than home directory\n");
			return;
		}
		if (chdir(paths[i]) < 0)
			snprintf(res, MAX_LINE, "cd: %s directory does not exist", paths[i]);
		else
		{
			snprintf(res, MAX_LINE, "empty");
			updateCurrentPath(user);
		}
	}
}

void pathForward(S_User *user, char *next, char *res)
{
	if (chdir(next) < 0)
		snprintf(res, MAX_LINE, "cd: %s directory does not exist", next);
	else
		snprintf(res, MAX_LINE, "empty");

	updateCurrentPath(user);
}

void assignUsername(S_User *user, char *newName, void *socket)
{
	// getting username from user for personalized experience
	if (newName == NULL)
	{

		if (socket == NULL)
		{
			printf("%s", get_username);
			fgets(user->name, MAX_USER_NAME, stdin);
		}
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

// input mode == 0 => stdin
// input mode == 1 => socket
void setUpUser(S_User *USER, void *socket)
{
	time(&USER->start_time); // start user time

	if (socket == NULL)
		assignUsername(USER, NULL, stdin);
	else
		assignUsername(USER, NULL, socket);
	setUpPaths(USER);
}

void help(char *res)
{
	snprintf(res, MAX_LINE, YEL "\n*** WELCOME TO THE HELP GUIDE ***\n"
								"----------------------------------\n"
								"Supported Commands:\n"
								"-------------------\n"
								"1 - help (prints the help manual for the shell)\n"
								"2 - exit (exits the shell)\n"
								"3 - cd <dst_path> (used to change the working directory)\n"
								"4 - pwd (prints the current working directory of the user <username/home/...>)\n"
								"5 - user [ -i, -n ]\n"
								"\t-n <new_username> (changes the username of the current user)\n"
								"\t-i (print user info)\n"
								"6 - ls, ps, cat ... (supports most UNIX commands with their arguments)\n"
								"7 - mkdir, rmdir, rm ... (supports most UNIX commands with their arguments)\n"
								"8 - pipe support for up to 21+ commands\n"
								"\n*** Jeffrey Joumjian - Maria Kantardjian - Reem Saado ***\n" RESET);
}

void printUserInfo(S_User user, char *res)
{

	// snprintf(res, sizeof(res), "user: %s\n", user.name);
	// snprintf(res, sizeof(res), "home dir: %s\n", user.HOME_DIR);
	// snprintf(res, sizeof(res), "current dir: %s\n", user.curr_path);

	time_t current_time;
	time(&current_time);
	snprintf(res, MAX_LINE,
			 "user: %s\n"
			 "home dir: %s\n"
			 "current dir: %s\n"
			 "time elapsed: %.3fs\n",
			 user.name, user.HOME_DIR, user.curr_path, (double)difftime(current_time, user.start_time));
}

// get input from user and put it in the cmd string
int getInput(char *cmd, void *socket)
{

	char buff[MAX_CMD_SIZE];

	if (socket == NULL)
	{
		if (fgets(buff, MAX_CMD_SIZE, stdin) > 0 && buff[0] != '\n')
		{
			String_copy(cmd, buff);
			return 1; // return true if successful scan
		}
	}
	else
	{
		if (read(*(int *)socket, buff, MAX_CMD_SIZE) > 0 && buff[0] != '\n')
		{
			String_copy(cmd, buff);
			return 1; // return true if successful scan
		}
	}

	return 0; // return false if empty
}

void execCustomCommand(char *args[], S_User *USER, void *socket)
{

	int wr = socket == NULL ? 1 : *(int *)socket;
	int wre = socket == NULL ? 1 : *(int *)socket;
	char res[MAX_LINE];

	if (String_EqualsIgnoreCase(args[0], "exit"))
		exit(0);

	else if (String_EqualsIgnoreCase(args[0], "cd"))
	{
		// if cd doesn't have arguments
		if (args[1] == NULL)
			snprintf(res, sizeof(res), "empty");
		else
		{
			// change directory and update curr_path
			if (strstr(args[1], "..") != NULL)
				pathBack(USER, args[1], res);
			else
				pathForward(USER, args[1], res);
		}
	}

	else if (String_EqualsIgnoreCase(args[0], "mkdir"))
	{
		for (int i = 0; i < MAX_ARGS_SIZE && args[i] != NULL; i++)
		{
			if (mkdir(args[i], 0770) < 0)
				snprintf(res, sizeof(res), "mkdir: %s already exists\n", args[1]);
			else
				snprintf(res, sizeof(res), "empty");
		}
	}
	else if (String_EqualsIgnoreCase(args[0], "rmdir"))
	{
		for (int i = 0; i < MAX_ARGS_SIZE && args[i] != NULL; i++)
		{
			if (rmdir(args[i]) < 0)
				snprintf(res, sizeof(res), "rmdir: %s does not exist\n", args[1]);
			else
				snprintf(res, sizeof(res), "empty");
		}
	}
	else if (String_EqualsIgnoreCase(args[0], "rm"))
	{
		for (int i = 0; i < MAX_ARGS_SIZE && args[i] != NULL; i++)
		{
			if (remove(args[i]) < 0)
				snprintf(res, sizeof(res), "rm: %s does not exist\n", args[1]);
			else
				snprintf(res, 6, "empty");
		}
	}

	// print working directory
	else if (String_EqualsIgnoreCase(args[0], "pwd"))
		snprintf(res, sizeof(res), "%s\n\n", USER->curr_path);

	// if cmd == user -n => change username
	else if (String_EqualsIgnoreCase(args[0], "user"))
	{
		if (args[1] == NULL)
			snprintf(res, sizeof(res), "%s is not a valid argument. user [ -i, -n ]\n", args[1]);
		else if (String_EqualsIgnoreCase(args[1], "-i"))
			printUserInfo(*USER, res);
		else if (String_EqualsIgnoreCase(args[1], "-n"))
			assignUsername(USER, args[2], stdin);
		else
			snprintf(res, sizeof(res), "%s is not a valid argument. user [ -i, -n ]\n", args[1]);
	}

	else if (String_EqualsIgnoreCase(args[0], "help"))
		help(res);
	write(wr, res, sizeof(res));

	printf(RESET);
}

void execCommand(char *args[], void *socket)
{
	int client_socket = *(int *)socket;
	// create child to run command
	pid_t pid = fork();

	if (pid < 0)
	{
		perror("failed to create child.\n");
		exit(1);
	}

	else if (pid == 0)
	{
		if (socket != NULL)
		{
			dup2(client_socket, 1);
			dup2(client_socket, 2);
			close(client_socket);
		}
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

// get the index of the next pipe or size of args if there is no pipe
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

void execPipedCommand(char *args[], char *piped_args[], char temp[], S_User *user, void *socket)
{

	int client_socket = *(int *)socket;

	// int wr = socket == NULL ? WRITE_END : client_socket;
	// int wre = socket == NULL ? 2 : client_socket;

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
			int dupped;
			if (socket != NULL)
			{
				dupped = dup2(client_socket, 1);
				dup2(client_socket, 2);
			}

			getArgsForCurrentExec(args, piped_args, start, end); // get the cmds between args[start] -> args[end] to execute them

			close(fd[READ_END]);
			dup2(fdd, READ_END); // get input from pipe instead of stdin
			if (i + 1 != num_cmds)
			{

				dup2(fd[WRITE_END], dupped); // write error to pipe instead of stdout
				dup2(fd[WRITE_END], dupped); // write output to pipe instead of stdout
			}
			close(fd[WRITE_END]);

			// first check if it's a custom command
			if (isCustomCommand(piped_args[0]))
			{
				execCustomCommand(args, user, NULL);
				exit(0);
			}
			else
			{
				// this is to make sure that ps buffers output correctly
				// if (String_EqualsIgnoreCase(piped_args[0], "ps") && String_EqualsIgnoreCase(piped_args[1], "aux"))
				// {
				// 	piped_args[1] = "aux";
				// 	for (int j = 2; j < MAX_ARGS_SIZE; j++)
				// 	{
				// 		if (piped_args[j] == NULL)
				// 		{
				// 			piped_args[j] = "--cols";
				// 			piped_args[j + 1] = "1000000000000000";
				// 			piped_args[j + 2] = NULL;
				// 			break;
				// 		}
				// 	}
				// }

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
