#include "shell.h"

void *handle_client(void *_CLIENT)
{
	CLIENT client = *(CLIENT *)_CLIENT;

	time_t _time;

	// log which client is connected
	_time = time(NULL);
	char time_str[30];
	String_copy(time_str, ctime(&_time));
	String_splitFirst(time_str, "\n");

	read(client.socket, client.user.name, MAX_USER_NAME);
	String_splitFirst(client.user.name, "\n");

	setUpUser(&client.user, NULL);

	printf("Client [%d]: {%s} has logged in at time {%s}\n", client.id, client.user.name, time_str);

	// while client is still connected
	while (client.socket > 0)
	{
		// read from client
		if (client.socket > 0)
		{
			write(client.socket, client.user.curr_path, MAX_PATH_SIZE);

			if (read(client.socket, client.cmd, MAX_CMD_SIZE) <= 0)
			{
				printf("Client [%d] terminated connection\n", client.id);
				close(client.socket);
				break;
			}

			printf("\tClient {%d}: %s %s $ %s\n", client.id, client.user.name, client.user.curr_path, client.cmd);

			bzero(stdin, sizeof(stdin));
			bzero(stderr, sizeof(stderr));

			// exec command
			// parse the command to see what to do
			// if pc == 0 => custom command (handled inside parse command)
			// if pc == 1 => simple command
			// if pc == 2 => piped command
			int pc = parseCommand(client.cmd, client.args, client.temp);

			if (pc == 0)
				execCustomCommand(client.args, &client.user, &client.socket);
			else if (pc == 1)
				execCommand(client.args, &client.socket);
			else if (pc == 2)
				execPipedCommand(client.args, client.piped_args, client.temp, &client.user, &client.socket);
		}
	}
	close(client.socket);
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{

	// create the server socket
	printf("Creating socket...\n");
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (server_socket < 0)
	{
		perror("Failed to create socket\n");
		exit(1);
	}

	// define server address and it's properties
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(SERVER_PORT);

	// bind the socket to our specified ip and port
	printf("Binding IP to socket...\n");
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

	int num_clients = 0;
	int client_socket = 0;

	printf("Listening for connections:\n");
	while (1)
	{
		// accept client, if accept fails, continue and accept another client
		client_socket = accept(server_socket, (struct sockaddr *)NULL, NULL);
		if (client_socket < 0)
			continue;

		++num_clients;
		if (fork() == 0)
		{

			// create new client and set up properties
			CLIENT client = {};
			S_User user = {};
			client.socket = client_socket;
			client.user = user;
			client.id = num_clients;

			pthread_t client_thread;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			pthread_create(&client_thread, &attr, handle_client, &client);
		}
	}
	close(server_socket);
}