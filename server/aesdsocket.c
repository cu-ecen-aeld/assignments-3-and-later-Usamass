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
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include "queue.h/queue.h"

#define BUF_SIZE 1024

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutx = PTHREAD_MUTEX_INITIALIZER;

typedef struct thread_data_t thread_data_s;
struct thread_data_t {
	int flag;
	pthread_t conn_id;
	int value;
	SLIST_ENTRY(thread_data_t) next_thread;
};

SLIST_HEAD(s_thread_list , thread_data_t) thr_head;




int sock = 0;
int conn_fd = 0;
ssize_t count = 0;
struct addrinfo *serverinfo = NULL;
struct sockaddr client;
socklen_t client_addrlen;
FILE* f_sock = NULL; // open socket for buffered reading
FILE* f_out = NULL;
pthread_attr_t thr_attr;
#define OUT_FILE "/var/tmp/aesdsocketdata"


static void err()
{
	fprintf(stderr, "%s\n", strerror(errno));
	exit(-1);
}


void timerHandler(int signo, siginfo_t *si, void *context) {
    syslog(LOG_INFO , "Timer expired!\n");

	char outstr[200];
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL) {
		perror("localtime");
		exit(EXIT_FAILURE);
	}

	if (strftime(outstr, sizeof(outstr), "%Y-%m-%d %H:%M:%S", tmp) == 0) {
		fprintf(stderr, "strftime returned 0");
		exit(EXIT_FAILURE);
	}
	
	pthread_mutex_lock(&mutx);

	f_out = fopen(OUT_FILE, "a+"); // open output file
	if (f_out == NULL) err();
	fprintf(f_out , "timestamp:%s\n" , outstr);
	// fputs(outstr, f_out); // write line to the file
	fflush(f_out);		// flush cached write to disk

	fclose(f_out);
	f_out = NULL;

	pthread_mutex_unlock(&mutx);

}

static void init_timer () 
{
	timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    struct sigaction sa;

    // Set up the timer signal handler
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerHandler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Set up the timer event
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timerid;

    // Create the timer
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        perror("timer_create");
        exit(EXIT_FAILURE);
    }

    // Set up the timer expiration time and interval
    its.it_value.tv_sec = 2;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 10;
    its.it_interval.tv_nsec = 0;

    // Arm the timer
    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }





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

static void register_intr () {
	// set up signal handler to catch SIGINT and SIGTERM and exit gracefully
	struct sigaction new_action;
	new_action.sa_handler = clean_up;
	new_action.sa_flags = 0;
	sigemptyset(&new_action.sa_mask);
	sigaction(SIGINT, &new_action, NULL);
	sigaction(SIGTERM, &new_action, NULL);
}

static void* thread_connection (void* arg) 
{
	thread_data_s* thread_data = (thread_data_s*)arg;
	
	
	// printf("thread id: %ld!\n" , thread_data->conn_id);
	printf("thread id: %ld!\n" , thread_data->conn_id);
	char *line = NULL;
	size_t len = 0;
	// open socket connection stream for buffered reading
	f_sock = fdopen(conn_fd, "rb");

	while (true) {
		

		if (f_sock) {
			if ((count = getline(&line, &len, f_sock)) != -1) {
				pthread_mutex_lock(&mutx);
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
				pthread_mutex_unlock(&mutx);

				fclose(f_sock);
				f_sock = NULL;
				close(conn_fd);
				conn_fd = 0;
				// syslog(LOG_INFO, "Closed connection from %s\n", inet_ntoa(addr_in->sin_addr));
				thread_data->flag = 1;
				return NULL;
			}
			else {
				err();
			}
		}
		else
		{
			err();
		}
		
	}
			
	
}

	


static void init_connection (const char PORT[]) 
{
	int status;
	struct addrinfo hints;
	
	const int reuse_enable = 1;

	memset(&client, 0, sizeof(client));
	memset(&client_addrlen, 0, sizeof(client_addrlen));

	// initialize addr hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// all interfaces on port 9000
	getaddrinfo(NULL , PORT, &hints, &serverinfo);

	// open the socket
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		err();

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_enable, sizeof(reuse_enable));
	setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse_enable, sizeof(reuse_enable));

	// bind to the port
	if ((status = bind(sock, serverinfo->ai_addr, serverinfo->ai_addrlen)) != 0)
		err();

	
	// open the socket for listening
	listen(sock, 5);

	


}

int main (int argc , char* argv[]) 
{
	

	thread_data_s* datap = NULL;
	SLIST_INIT(&thr_head);

    pthread_attr_init(&thr_attr);
	register_intr();
	init_connection("9000");
	// // this is a really lazy way to handle the daemon option but it meets the spec ;)
	if (argc == 2 && strcmp(argv[1], "-d") == 0 && fork()) {
		exit(0);
	}
	init_timer();

	pthread_t thread_id;
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);

	while (true) {
		conn_fd = accept(sock, &client, &client_addrlen);
		if (conn_fd != -1) {
			datap = malloc(sizeof(thread_data_s));
			int result = pthread_create(&thread_id , &thread_attr , (void*)thread_connection , datap);
			datap->flag = 0;
			datap->conn_id = thread_id;
			SLIST_INSERT_HEAD(&thr_head , datap , next_thread);

			if (result == 0) printf("thread has initialized!\n");

			SLIST_FOREACH(datap , &thr_head , next_thread) {
				printf("thread DATA: %ld\n" , datap->conn_id);
				if (datap->flag) {
					printf("thread %ld joined\n" , datap->conn_id);
					pthread_join(datap->conn_id, NULL);

				}
				
			}
				

		}


	}
	

	return 0;
}

