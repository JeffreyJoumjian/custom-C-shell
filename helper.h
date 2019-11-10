#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>

char *String_remove(char *str, char *setOfDelimeters)
{
	return strtok(str, setOfDelimeters);
}
char *String_concat(char *str, char *setOfDelimeters)
{
	return strcat(str, setOfDelimeters);
}

int String_isSubstring(char *big, const char *little)
{
	return strcasestr(big, little) != NULL;
}

char *String_copy(char *to, char *from)
{
	return strcpy(to, from);
}

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
	String_remove(cmd, "\n\r");
}