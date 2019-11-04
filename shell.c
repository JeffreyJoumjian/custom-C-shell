#include "shell.h"

int main()
{
	int close_server = 0;
	// set up commands
	char *args[MAX_ARGS_SIZE];
	char cmd[MAX_CMD_SIZE];

	// set up user and paths
	S_User USER;
	int au = assignUsername(USER.name, MAX_USER_NAME, NULL);
	setUpPaths(&USER);

	while (1)
	{
		// shell $ username:
		fputs(SHELL_INDICATOR, stdout);
		if (au == 0)
			fputs(USER.name, stdout);

		// get command from terminal
		if (fgets(cmd, MAX_CMD_SIZE, stdin) > 0)
		{
			// create new child to run command
			pid_t pid = fork();

			if (pid < 0)
			{
				perror("failed to create a child process");
				exit(1);
			}

			// run command in new child
			else if (pid == 0)
			{
				// make command ready to pass it to execvp, if < 0 => cmd == exit
				if (parseCommand(cmd, args) < 0 || strcasecmp(cmd, "exit") == 0)
					return 12;

				// change directory command
				else if (strcasecmp(args[0], "cd") == 0)
				{
					// if cd doesn't have an arg or is trying to go up server directory
					if (args[1] == NULL)
						continue;

					// change directory and update curr_path
					else if (strstr(args[1], "..") != NULL)
						pathBack(&USER, args[1]);
					else
						pathForward(&USER, args[1]);
				}

				// print working directory
				else if (strcasecmp(args[0], "pwd") == 0)
					printf("%s\n", USER.curr_path);

				// if cmd == user -n => change username
				else if (strcasecmp(args[0], "user") == 0 && strcasecmp(args[1], "-n") == 0)
					assignUsername(USER.name, MAX_USER_NAME, args[2]);

				else if (hasPipes(cmd) > 0)
				{
					// work with pipes
				}
				else
					execvp(args[0], args);
			}

			else if (pid > 0)
			{
				int status = 0;
				pid = wait(&status);

				printf("\n");

				// exit if exit command is used
				if (status / 256 == 12)
				{
					printf("Server closed\n");
					exit(0);
				}
			}
		}
	}
	return 0;
}