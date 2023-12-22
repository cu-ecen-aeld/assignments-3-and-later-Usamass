#include <stdio.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

#define DEBUG_LOG(msg,...) syslog(LOG_DEBUG,"SOCK_DEBUG: " msg "\n", ##__VA_ARGS__)
#define USER_LOG(msg,...) syslog(LOG_USER,"SOCK_DEBUG: " msg "\n", ##__VA_ARGS__)
#define ERROR_LOG(msg,...) syslog(LOG_ERR ,"SOCK_ERROR: " msg "\n" , ##__VA_ARGS__)

#define MAX_BUFFER_LEN (1024 * 1024 *2)

char client_ip[INET_ADDRSTRLEN] = {0};
char* recv_buffer = NULL;
int sock_fd , s , conn_fd , file_fd;
uint8_t daemon_action = 0;

char* file_path = "/var/tmp/aesdsocketdata";


static void signal_handler(int signo) 
{
    if (signo == SIGINT || signo == SIGTERM) {
        USER_LOG("Cought signal exiting\n");
        if (conn_fd >= 0) close(conn_fd); 
        if (file_fd >= 0) close(file_fd); 
        free(recv_buffer);
        remove("/var/tmp/aesdsocketdata");
        
    }
    closelog(); 
    exit(EXIT_SUCCESS);
    

}

static void make_daemon() 
{
    pid_t pid , sid;
    pid = fork();

    if (pid == -1) {
        ERROR_LOG("unable to fork!\n");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        USER_LOG("exiting from parent process!\n");
        exit(EXIT_SUCCESS);
    }

    /* change the file mode mask*/
    umask(0);

    /* create a new session ID for the child process*/
    sid = setsid();
    if (sid < 0) {
        ERROR_LOG("setsid failed\n");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);



}


int main (int argc , char* argv[]) 
{

      /*Signal handling*/
    signal(SIGINT , signal_handler);
    signal(SIGTERM , signal_handler);

    if (argc == 2 && strcmp(argv[1] , "-d") == 0) daemon_action = 1; // setting daemon_action flag.

    ssize_t data_size = 0;
    
    /*socket structures*/
    struct addrinfo addr_info , *res;
    struct sockaddr client_info;
    struct sockaddr_in* ipv4_client_addr = (struct sockaddr_in*)&client_info;
    memset(&addr_info , 0 , sizeof(addr_info));

    /*Opening necessery file*/
    openlog("socket" , LOG_CONS | LOG_PID , LOG_USER);
    USER_LOG("This is an assignment5");
    file_fd = open(file_path, O_CREAT | O_RDWR | O_APPEND);
    if (file_fd == -1) {
        ERROR_LOG("error ocurred while opening file!\n");
        closelog();
        exit(EXIT_FAILURE);

    }

    sock_fd = socket(AF_INET , SOCK_STREAM , 0);

    if (sock_fd == -1) {
        ERROR_LOG("error ocurred while creating socket!\n");
        closelog();
        exit(EXIT_FAILURE);
    }

    addr_info.ai_family = AF_UNSPEC;
    addr_info.ai_socktype = SOCK_STREAM;
    addr_info.ai_flags = AI_PASSIVE;

    s = getaddrinfo(NULL , "9000" , &addr_info , &res);
    if (s != 0) {
        ERROR_LOG("error in getaddrinfo\n");
        freeaddrinfo(res);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (bind(sock_fd , res->ai_addr , res->ai_addrlen) != 0) {
        ERROR_LOG("error in binding\n");
        freeaddrinfo(res);
        closelog();
        exit(EXIT_FAILURE);

    }
    freeaddrinfo(res);
    /* if '-d' (daemon) option is selected */
    if (daemon_action) {
        USER_LOG("making this process a daemon!\n");
        make_daemon();
    }

    if (listen(sock_fd , 5) == -1) {
        ERROR_LOG("error in listning to socket\n");
        closelog();
        exit(EXIT_FAILURE);

    }

    socklen_t client_len = sizeof(struct sockaddr);

    recv_buffer = (char*)malloc(sizeof(char) * MAX_BUFFER_LEN);
    if (recv_buffer == NULL) {
        ERROR_LOG("unable to allocate memory\n");
        closelog();
        exit(EXIT_FAILURE);

    }
    memset(recv_buffer , 0 , sizeof(char) * MAX_BUFFER_LEN);
    while (1) {
        // printf("waiting to accept connection!\n");
        conn_fd = accept(sock_fd , &client_info , &client_len);
        inet_ntop(AF_INET , &(ipv4_client_addr->sin_addr) , client_ip , sizeof(client_ip));
        USER_LOG("Accepted connection from: %s" , client_ip);
    
        /*receiving data from socket*/
        data_size = recv(conn_fd , recv_buffer , MAX_BUFFER_LEN , 0);

        if(write(file_fd , recv_buffer , data_size) == -1) {
            ERROR_LOG("unable to write to the socket\n");
            free(recv_buffer);
            exit(EXIT_FAILURE);
        }

        if (strchr(recv_buffer , '\n') != NULL) {
            memset(recv_buffer , 0 , sizeof(char) * MAX_BUFFER_LEN);
            // off_t file_size = lseek(file_fd , 0 , SEEK_END); // getting size of file.
            lseek(file_fd , 0 , SEEK_SET);  // pointing to beginning of the file.

            int bytes_read = read(file_fd , recv_buffer , MAX_BUFFER_LEN);

            while (bytes_read > 0) {
                //printf("bytes read: %d \n file data: %s \n cur position: %ld \n buffer size: %ld" ,bytes_read , recv_buffer , lseek(file_fd , 0 , SEEK_CUR) , (sizeof(recv_buffer)* MAX_BUFFER_LEN));
                ssize_t bytes_send = send(conn_fd , recv_buffer , (size_t)bytes_read , 0);
                // printf("bytes sent: %ld\n" , bytes_send);
                bytes_read = read(file_fd , recv_buffer , MAX_BUFFER_LEN);
                
            }

            memset(recv_buffer , 0 , sizeof(char) * MAX_BUFFER_LEN); // reset buffer for next use.
    
        }
        USER_LOG("closed connection from: %s" , client_ip);
        close(conn_fd);
        
    }

    return 0;
}
 