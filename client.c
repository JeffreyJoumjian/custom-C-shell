#include "shell.h"

int main()
{

	// SETTING UP CLIENT CONNECTION

	// create client socket
	int client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket < 0)
	{
		perror("Failed to create client socket\n");
		exit(1);
	}

	// define server address and it's properties to connect to it
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);

	// try to establish connection with server
	int inet = inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);

	if (inet <= 0)
	{
		perror("error connecting to the server - ");
		printf("%s:%d\n", SERVER_IP, SERVER_PORT);
		exit(1);
	}

	if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("Error establishing socket connection\n");
		exit(1);
	}
	// client is connected if here
	CLIENT client = {};

	// greet client by asking for username
	printf("%s", get_username);
	fgets(client.user.name, MAX_USER_NAME, stdin);
	write(client_socket, client.user.name, MAX_USER_NAME);

	// set up client properties
	setUpUser(&client.user, NULL);

	// while client is still connected
	while (client_socket > 0)
	{

		if (client_socket > 0)
		{
			// print username and $
			// if username isn't empty
			if (client.user.name[0] != '\n')
			{
				printf(RED "%s", client.user.name);
				printf(RESET);
			}
			printf(YEL "~%s ", client.user.curr_path);
			printf(RESET);
			printf(GRN "%s", SHELL_INDICATOR RESET);

			// read from client
			while (fgets(client.cmd, MAX_LINE, stdin))
			{
				client.cmd[strlen(client.cmd) - 1] = '\0';
				if (strlen(client.cmd) > 0)
					break;
				// print username and $
				printf(YEL "~%s ", client.user.curr_path);
				printf(RESET);
				printf(GRN "%s", SHELL_INDICATOR RESET);
			}

			// if input not empty
			// if exit => disconnect from server
			if (String_EqualsIgnoreCase(client.cmd, "exit"))
			{
				close(client_socket);
				break;
			}
			if (write(client_socket, client.cmd, MAX_CMD_SIZE) < 0)
				perror("Error writing to server.\n");

			// read response from server
			// if (read(client_socket, client.temp, MAX_LINE) < 0)
			// 	perror("Error reading from server.\n");
			char x;
			int i = 0;
			while (i++ < 4)
			{
				int n = read(client_socket, client.temp, MAX_LINE);
				if (i == 4)
					printf("%s", client.temp);
			}
		}
	}
	close(client_socket);
	return 0;
}