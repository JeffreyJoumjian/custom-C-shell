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

	// CLIENT IS NOW CONNECTED
	CLIENT client;
	// char * input = &client.input;

	// greet client
	printf("%s", CLIENT_GREET);

	while (1)
	{
		// get client input
		if (fgets(client.input, MAX_LINE, stdin) > 0)
		{
			// remove whitespace
			strtok(client.input, "\n\r");

			// send client input to server
			write(client_socket, client.input, sizeof(client.input));

			if (strcasecmp(client.input, "exit") == 0)
				break;

			// get response from server
			read(client_socket, client.res, sizeof(client.res));
			printf("%s\n", client.res);
		}
	}

	close(client_socket);
}