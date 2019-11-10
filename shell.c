#include "shell.h"

int main()
{
	int close_server = 0;
	// set up commands
	char *args[MAX_ARGS_SIZE];
	char cmd[MAX_CMD_SIZE];

	// set up user and paths
	S_User USER;
	int au = assignUsername(&USER, NULL);
	setUpPaths(&USER, NULL);

	while (1)
	{
		// shell $ username:
		fputs(SHELL_INDICATOR, stdout);
		if (au == 0)
			fputs(USER.name, stdout);

		// get command from terminal and check if it's not an empty string
		if (getInput(cmd))
		{
			// parse the command to see what to do
			int pc = parseCommand(cmd, args);

			// if failed to parse => command is exit => exit shell
			if (!pc)
				return EXIT_CMD;

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
				assignUsername(&USER, args[2]);

			// if it's none of the above commands, create a child and execute the command in the child
			else
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
					// if no pipes then just exec cmd
					if (hasPipes(cmd) < 0)
						execvp(args[0], args);
					else
					{
						// work with pipes
					}
					exit(0);
				}

				else if (pid > 0)
				{
					pid = wait(0);
					printf("\n");
				}
			}
		}
		// if empty string then continue scanning
		else
			continue;
	}
	// return 0;
}