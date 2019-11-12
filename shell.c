#include "shell.h"

int main()
{
	// set up commands
	// char cmd[MAX_CMD_SIZE];
	// char *args[MAX_ARGS_SIZE];
	// int pipe_locs[MAX_PIPE_SIZE];
	// char pipe_types[MAX_PIPE_SIZE];
	COMMAND command;

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
		if (getInput(command.cmd))
		{
			// parse the command to see what to do
			// if pc == 0 => custom command (handled inside parse command)
			// if pc == 1 => simple command
			// if pc == 2 => piped command
			int pc = parseCommand(&command);

			if (pc == 0)
				execCustomCommand(command.args, &USER);
			else if (pc == 1)
				execCommand(command.args);
			else
			{
				// execPipedCommand(command.args,command.pipe_locs,command.pipe_types);
			}
		}
		else
			continue;
	}
}