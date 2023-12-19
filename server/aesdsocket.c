#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE (1024 * 1024)
#define FILE_PATH ("/var/tmp/aesdsocketdata")

static bool disconnect = false;
static int serverfd;
static int clientfd;
static char *data_buf;
static char ipaddr[INET_ADDRSTRLEN];

static void signal_callback(int signal_number)
{
	if (signal_number == SIGINT || signal_number == SIGTERM)
	{
		syslog(LOG_INFO, "Closed connection from %s\n", ipaddr);
		shutdown(serverfd, SHUT_RDWR);
		shutdown(clientfd, SHUT_RDWR);
		remove("/var/tmp/aesdsocketdata");
		disconnect = true;
	}
}

static bool send_data(void)
{
	FILE* file;

	if ((file = fopen(FILE_PATH, "r")) == NULL)
	{
		//printf ("file open failed\n");
		return false;
	}

	while(fgets(data_buf, BUF_SIZE, file) != NULL)
	{
		if (send(clientfd, data_buf, strlen(data_buf), 0) == -1)
		{
			//printf("send is failed\n");
			return false;
		}
	}
								
	if (fclose(file) != 0)
	{
		//printf("failed to close file");
		return false;
	}

	return true;
}

static bool write_file(int data_size)
{
	FILE* file;
	
	if ((file = fopen(FILE_PATH, "a+")) == NULL)
	{
		//printf("Failed to open write file");
		return false;
	}

	char* tmp_buf = data_buf;
	for (int i = 0; i < data_size; i++)
	{
		if (data_buf[i] == '\n')
		{
			data_buf[i] = '\0';
			if (fprintf(file, "%s\n", tmp_buf) < 0)
			{
				//printf("Failed to write file");
				return false;
			}

			tmp_buf = &data_buf[i + 1];
		}
	}

	if (fclose(file) != 0)
	{
		//printf("failed to close file");
		return false;
	}

	return true;
}
								

int main(int argc, char * argv[])
{
	bool daemon_bool = false;
	int pid = 0;
	int result = 0;
	struct sigaction signal_action;
	struct addrinfo hints;

	if ((argc == 2) && (!strcmp(argv[1], "-d")))
		daemon_bool = true;

	memset(&signal_action, 0, sizeof(struct sigaction));
	signal_action.sa_handler = signal_callback;

	if (sigaction(SIGTERM, &signal_action, NULL) != 0)
	{
		perror("fail to register SIGTERM signal callback");
		return -1;
	}

	if (sigaction(SIGINT, &signal_action, NULL) != 0)
	{
		perror("fail to register SIGINT signal callback");
		return -1;
	}
			
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	//hints.ai_flags = AI_PASSIVE;

	if ((serverfd = socket(hints.ai_family, hints.ai_socktype, 0)) == -1)
	{
		perror("fail to create socket");
		return -1;
	}
	
	struct addrinfo *servinfo;

	int option = 1;
	if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) == -1)
	{
		perror("fail to set socket option");
		return -1;
	}
	
	int getaddrinfo_err;
	if ((getaddrinfo_err = getaddrinfo(NULL, "9000", &hints, &servinfo)))
	{
		printf("%d\n", getaddrinfo_err);
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_err));
		return -1;
	}

	const char* p_ip_addr;
	if ((p_ip_addr = inet_ntop(AF_INET, servinfo->ai_addr, ipaddr, INET_ADDRSTRLEN)) == NULL)
	{
		perror("fail to convert ipv4 string");
		return -1;
	}
		
	syslog(LOG_INFO, "Accepted connection from %s\n", p_ip_addr);
	
	if (bind(serverfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		perror("fail to bind socket");
		return -1;
	}
	
	freeaddrinfo(servinfo);

	if (daemon_bool)
	{
		pid = fork();
		
		if(pid < 0)
		{
			syslog(LOG_DEBUG, "FORK failed");
			exit(EXIT_FAILURE);
		}

		if(pid > 0)
		{
			syslog(LOG_DEBUG, "Parent exiting");
			exit(EXIT_SUCCESS);
		}
	}
		

	if (pid == 0)
	{
		if (listen(serverfd, 2) == -1)
		{
			perror("fail to listen socket");
			return -1;
		}

		struct sockaddr sockaddr_connected;
		socklen_t sockaddrlen_connected = sizeof(struct sockaddr);
		
		openlog (NULL, 0, LOG_USER);
		data_buf = malloc(sizeof(char) * BUF_SIZE);

		while (!disconnect)
		{
			if ((clientfd = accept(serverfd, &sockaddr_connected, &sockaddrlen_connected)) == -1)
			{
				perror("fail to listen socket");
				result = -1;
				break;
			}	

			while(true)
			{
				int num_bytes_received;

				if ((num_bytes_received = recv(clientfd, data_buf, BUF_SIZE, 0)) < 0)
				{
					perror("fail to receive data");
					disconnect = true;
					result = -1;
					break;
				}
				else if (num_bytes_received == 0)
				{
					break;
				}
				
				if (num_bytes_received > BUF_SIZE)
				{
					fprintf(stderr, "packet is exceed the buffer size!");
					disconnect = true;
					result = -1;
					break;
				}
				
				if (write_file(num_bytes_received) == false)
				{
					fprintf(stderr, "fail to write file");
					disconnect = true;
					result = -1;
					break;
				}
		
				if (send_data() == false)
				{
					fprintf(stderr, "fail to send data");
					disconnect = true;
					result = -1;
					break;
				}
			}
		}
		
		free(data_buf);
		closelog();
	}

	return result;
}