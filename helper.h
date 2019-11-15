#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include <stdbool.h>
#include <string.h>
#include <time.h>

int String_Equals(char *str1, char *str2)
{
	return strcmp(str1, str2) == 0;
}
int String_EqualsIgnoreCase(char *str1, char *str2)
{
	return strcasecmp(str1, str2) == 0;
}

char *String_splitFirst(char *str, char *seperators)
{
	return strtok(str, seperators);
}

char *String_remove(char *big, char *little)
{
	if (strstr(big, little))
		return strncpy(big, big + strlen(little), strlen(big) - 1);
	return NULL;
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
	String_splitFirst(cmd, "\n\r");
}

void clearStringArray(char *args[], int size)
{
	for (int i = 0; i < size && args[i] != NULL; i++)
		args[i] = NULL;
}