#include "shell.h"

int main()
{
	// set up commands
	char cmd[MAX_CMD_SIZE];			 // string to store scanned command
	char temp[MAX_CMD_SIZE];		 // string to be used as a temporary string while separating the cmd into args
	char *args[MAX_ARGS_SIZE];		 // string[] to contain the individual parts of each command
	char *piped_args[MAX_ARGS_SIZE]; // string[] to be used for pipe execution

	// set up user and paths
	S_User USER;
	setUpUser(&USER);

	printf("use \"help\" command for instructions\n");

	while (1)
	{
		// shell $ username:
		fputs(SHELL_INDICATOR, stdout);

		// if username isn't empty
		if (USER.name[0] != '\n')
			fputs(USER.name, stdout);

		// get command from terminal and check if it's not an empty string
		if (getInput(cmd))
		{
			// parse the command to see what to do
			// if pc == 0 => custom command (handled inside parse command)
			// if pc == 1 => simple command
			// if pc == 2 => piped command
			int pc = parseCommand(cmd, args, temp);

			if (pc == 0)
				execCustomCommand(args, &USER);
			else if (pc == 1)
				execCommand(args);
			else if (pc == 2)
			{
				execPipedCommand(args, piped_args, &USER);
			}
		}
		else
			continue;
	}
}