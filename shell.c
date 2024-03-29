#include "shell.h"

int main()
{
	// set up commands
	char cmd[MAX_CMD_SIZE];			 // string to store scanned command
	char temp[MAX_CMD_SIZE];		 // string to be used as a temporary string while separating the cmd into args
	char *args[MAX_ARGS_SIZE];		 // string[] to contain the individual parts of each command
	char *piped_args[MAX_ARGS_SIZE]; // string[] to be used for pipe execution

	printf(YEL);
	// set up user and paths
	S_User USER;
	setUpUser(&USER, stdin);

	printf("\nuse \"help\" command for instructions\n\n");

	while (1)
	{
		printf(RESET);
		// shell $ username:
		// fputs(SHELL_INDICATOR, stdout);
		// printf(RESET);

		// if username isn't empty
		if (USER.name[0] != '\n')
		{
			printf(RED "%s", USER.name);
			printf(RESET);
		}
		// fputs(USER.name, stdout);

		// print username and $
		printf(YEL "~%s ", USER.curr_path);
		printf(RESET);
		printf(GRN "%s", SHELL_INDICATOR RESET);
		// get command from terminal and check if it's not an empty string
		if (getInput(cmd, NULL))
		{
			// parse the command to see what to do
			// if pc == 0 => custom command (handled inside parse command)
			// if pc == 1 => simple command
			// if pc == 2 => piped command
			int pc = parseCommand(cmd, args, temp);

			if (pc == 0)
				execCustomCommand(args, &USER, NULL);
			else if (pc == 1)
			{
				execCommand(args, NULL);
			}
			else if (pc == 2)
			{
				execPipedCommand(args, piped_args, temp, &USER, NULL);
			}
		}
		else
			continue;
	}
}