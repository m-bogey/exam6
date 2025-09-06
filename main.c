#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct s_client {
	int id;
	char buf[200000];
	char msg[200000];
} t_client;

t_client *clients[1024];
fd_set active, read_fds, write_fds;
int max_fd = 0, next_id = 0, server_fd = 0;

void fatal() {
	write(2, "Fatal error\n", strlen("Fatal error\n"));
	exit(1);
}

void send_all(int except_fd, char *msg) {
	for (int fd = 0; fd <= max_fd; ++fd) {
		if (FD_ISSET(fd, &write_fds) && fd != except_fd) {
			send(fd, msg, strlen(msg), 0);
		}
	}
}

void new_connection() {
	struct sockaddr_in cli;
	socklen_t len = sizeof(cli);
	int client_fd = accept(server_fd, (struct sockaddr*)&cli, &len);
	if (client_fd < 0)
		fatal();

	// if (!(clients[client_fd] = calloc(1, sizeof(t_client))))
	// 	fatal();
	clients[client_fd]->id = next_id++;

	FD_SET(client_fd, &active);
	if (client_fd > max_fd)
		max_fd = client_fd;

	char buff[64];
	sprintf(buff, "server: client %d just arrived\n", clients[client_fd]->id);
	send_all(client_fd, buff);
}

void client_left(int fd)
{
	char buff[64];
	sprintf(buff, "server: client %d just left\n", clients[fd]->id);
	send_all(fd, buff);

	close(fd);
	free(clients[fd]);
	clients[fd] = NULL;
	FD_CLR(fd, &active);
}

void handle_msg(int fd)
{
	char recv_buf[4096];
	int r = recv(fd, recv_buf, sizeof(recv_buf) - 1, 0);
	if (r <= 0)
    {
		client_left(fd);
		return;
	}
	recv_buf[r] = '\0';
	sprintf(clients[fd]->msg, "client %d: %s\n", clients[fd]->id, recv_buf);
	send_all(fd, clients[fd]->msg);
}

int main(int ac, char **av) {
	if (ac != 2)
    {
		write(2, "Wrong number of arguments\n", 27);
		exit(1);
	}

	struct sockaddr_in servaddr;

	FD_ZERO(&active);
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		fatal();

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	if (bind(server_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		fatal();
	if (listen(server_fd, 128) < 0)
		fatal();

	FD_SET(server_fd, &active);
	max_fd = server_fd;

	while (1) {
		read_fds = write_fds = active;
		if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) < 0)
			continue;

		for (int fd = 0; fd <= max_fd; ++fd) {
			if (FD_ISSET(fd, &read_fds)) {
				if (fd == server_fd)
					new_connection();
				else
					handle_msg(fd);
			}
		}
	}
	return 0;
}
