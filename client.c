#include "shell.h"

int main()
{

	// SETTING UP CLIENT CONNECTION

	// create client socket
	printf("Creating socket...\n");
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
	printf("Finding server...\n");
	int inet = inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);

	if (inet <= 0)
	{
		perror("error connecting to the server - ");
		printf("%s:%d\n", SERVER_IP, SERVER_PORT);
		exit(1);
	}

	printf("Connecting to server...\n");
	if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("Error establishing socket connection\n");
		exit(1);
	}

	printf("!!! Connection Established !!!\n");
	// client is connected if here
	CLIENT client = {};

	// greet client by asking for username
	printf(YEL "%s", get_username);
	printf(RESET);
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
			read(client_socket, client.user.curr_path, MAX_PATH_SIZE);
			printf(RESET);
			printShell(&client);

			// read from client
			clearString(client.cmd, strlen(client.cmd));
			while (fgets(client.cmd, MAX_LINE, stdin))
			{
				client.cmd[strlen(client.cmd) - 1] = '\0';
				if (strlen(client.cmd) > 0)
					break;
				// print username and $
				printShell(&client);
			}

			// if input not empty
			// if exit => disconnect from server
			if (String_EqualsIgnoreCase(client.cmd, "exit"))
			{
				close(client_socket);
				break;
			}

			// if change user name => change locally too
			if (strcasestr(client.cmd, "user -n"))
			{
				parseCommand(client.cmd, client.args, client.temp);
				assignUsername(&client.user, client.args[2], NULL);
			}

			if (write(client_socket, client.cmd, MAX_CMD_SIZE) < 0)
				perror("Error writing to server.\n");

			// read response from server
			// if (read(client_socket, client.temp, MAX_LINE) < 0)
			// 	perror("Error reading from server.\n");
			// while (i > 0)
			// {
			clearString(client.temp, strlen(client.temp));
			if (read(client_socket, client.temp, MAX_LINE) > 0)
			{

				if (!String_EqualsIgnoreCase(client.temp, "empty"))
					printf("%s", client.temp);
			}

			// break;
			// }
		}
	}
	close(client_socket);
	return 0;
}