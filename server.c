#include "shell.h"

int main(int argc, char **argv)
{

	// create the server socket
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (server_socket < 0)
	{
		perror("Failed to create socket\n");
		exit(1);
	}

	// define server address abd it's properties
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(SERVER_PORT);

	// bind the socket to our specified ip and port
	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("Error Binding\n");
		exit(1);
	}

	// listen on that ip and port
	if (listen(server_socket, BACKLOG) < 0)
	{
		perror("Error Listening\n");
		exit(1);
	}

	int numClients = 0;
	int client_socket = 0;

	printf("Listening for connections:\n");
	while (1)
	{
		// accept client, if accept fails, continue and accept another client
		client_socket = accept(server_socket, (struct sockaddr *)NULL, NULL);
		if (client_socket < 0)
			continue;

		// create new client and set up properties
		CLIENT client;
		client.id = ++numClients;
		strcpy(client.res, CLIENT_RESPONSE);

		// log which client is connected
		printf("client %d has connected to the server\n", client.id);

		while (1)
		{

			// read cmd from client unless it's exit
			if (read(client_socket, client.input, sizeof(client.input)) > 0)
				printf("\tclient %d:\n\t\t%s\n", client.id, client.input);

			if (strcasecmp(client.input, "exit") == 0)
			{
				printf("client %d terminated connection \n", client.id);
				break;
			}
			// send response to client
			strcat(client.res, client.input);
			write(client_socket, client.res, sizeof(client.res));

			// clear client response and input to avoid conflicts
			strcpy(client.res, CLIENT_RESPONSE);
			strcpy(client.input, "\0");
		}
		close(client_socket);
	}
}