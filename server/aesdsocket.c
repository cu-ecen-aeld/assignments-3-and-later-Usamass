#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

const int BUF_SIZE = 1024;
const char *OUT_FILE = "/var/tmp/aesdsocketdata";

int sock = 0;
int conn_fd = 0;
ssize_t count = 0;
struct addrinfo *serverinfo = NULL;
FILE *f_out = NULL;	 // open output file
FILE *f_sock = NULL; // open socket for buffered reading

static void err()
{
	fprintf(stderr, "%s\n", strerror(errno));
	exit(-1);
}

static void clean_up(int sig_num)
{
	syslog(LOG_INFO, "%s\n", "Caught signal, exiting");
	if (f_sock)
		fclose(f_sock);
	if (conn_fd)
		close(conn_fd);
	if (f_out)
		fclose(f_out);
	if (sock)
		close(sock);
	if (serverinfo)
		freeaddrinfo(serverinfo);
	remove(OUT_FILE);
	exit(0);
}

int main(int argc, char *argv[])
{
	int status;
	struct addrinfo hints;
	struct sockaddr client;
	socklen_t client_addrlen;
	const int reuse_enable = 1;

	// set up signal handler to catch SIGINT and SIGTERM and exit gracefully
	struct sigaction new_action;
	new_action.sa_handler = clean_up;
	new_action.sa_flags = 0;
	sigemptyset(&new_action.sa_mask);
	sigaction(SIGINT, &new_action, NULL);
	sigaction(SIGTERM, &new_action, NULL);

	memset(&client, 0, sizeof(client));
	memset(&client_addrlen, 0, sizeof(client_addrlen));

	// initialize addr hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// all interfaces on port 9000
	getaddrinfo("0.0.0.0", "9000", &hints, &serverinfo);

	// open the socket
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		err();

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_enable, sizeof(reuse_enable));
	setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse_enable, sizeof(reuse_enable));

	// bind to the port
	if ((status = bind(sock, serverinfo->ai_addr, serverinfo->ai_addrlen)) != 0)
		err();

	// this is a really lazy way to handle the daemon option but it meets the spec ;)
	if (argc == 2 && strcmp(argv[1], "-d") == 0 && fork()) {
		exit(0);
	}

	// open the socket for listening
	listen(sock, 1);

	while (1)
	{
		conn_fd = accept(sock, &client, &client_addrlen);
		if (conn_fd)
		{
			char *line = NULL;
			size_t len = 0;

			// log IP address
			struct sockaddr_in *addr_in = (struct sockaddr_in *)&client;
			syslog(LOG_INFO, "Accepted connection from %s\n", inet_ntoa(addr_in->sin_addr));

			// open socket connection stream for buffered reading
			f_sock = fdopen(conn_fd, "rb");

			if (f_sock)
			{
				if ((count = getline(&line, &len, f_sock)) != -1)
				{
					f_out = fopen(OUT_FILE, "a+"); // open output file
					if (f_out == NULL)
						err();

					fputs(line, f_out); // write line to the file
					fflush(f_out);		// flush cached write to disk
					free(line);			// release memory

					{ // read in the entire file and send it back over the socket
						char buf[BUF_SIZE];
						size_t n = 0;

						fseek(f_out, 0, SEEK_SET); // start at the beginning
						while ((n = fread(buf, sizeof(char), BUF_SIZE, f_out)) > 0)
							send(conn_fd, buf, n, 0);
						fclose(f_out);
						f_out = NULL;
					}

					fclose(f_sock);
					f_sock = NULL;
					close(conn_fd);
					conn_fd = 0;
					syslog(LOG_INFO, "Closed connection from %s\n", inet_ntoa(addr_in->sin_addr));
				}
			}
			else
			{
				err();
			}
		}
		else
		{
			err();
		}
	}

	return 0;
}