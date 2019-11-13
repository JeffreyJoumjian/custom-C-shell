#include "shell.h"

int main()
{
	// set up commands
	char cmd[MAX_CMD_SIZE];
	char *args[MAX_ARGS_SIZE];
	char *piped_args[MAX_ARGS_SIZE];

	// set up user and paths
	S_User USER;
	setUpUser(&USER);

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
			int pc = parseCommand(cmd, args);

			if (pc == 0)
				execCustomCommand(args, &USER);
			else if (pc == 1)
				execCommand(args);
			else
			{
				// execPipedCommand(args,piped_args);
			}
		}
		else
			continue;
	}
}