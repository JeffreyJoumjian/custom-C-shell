#include "shell.h"

int main()
{
	int close_server = 0;
	// set up commands
	char *args[MAX_ARGS_SIZE];
	char cmd[MAX_CMD_SIZE];

	// set up user and paths
	S_User USER;
	USER.curr_path[0] = '\0';
	getcwd((char *)USER.SHELL_DIR, MAX_PATH_SIZE);
	assignUsername(&USER, NULL);
	setUpPaths(&USER);
	// chdir("home");

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
				// execPipedCommand();
			}

			// if (pc == 2)
			// 	execPipedCommand(args);
		}
		else
			continue;
		// return 0;
	}
}