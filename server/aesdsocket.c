#include <stdio.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

#define DEBUG_LOG(msg,...) syslog(LOG_DEBUG ,"SOCK_DEBUG: " msg "\n" , ##__VA_ARGS__)
#define USER_LOG(msg,...) syslog(LOG_USER ,"SOCK_USER: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) syslog(LOG_ERR ,"SOCK_ERROR: " msg "\n" , ##__VA_ARGS__)

char msg[200] = "connected to the server\n";
char client_ip[INET_ADDRSTRLEN] = {0};
char recv_buffer[1024] = {0};
int sock_fd , s , conn_fd;

char file_path[] = "/var/tmp/aesdsocketdata";


static void signal_handler() 
{
    printf("server shutdown!\n");
    close(conn_fd);
    exit(EXIT_SUCCESS);

}

static void infinit_write(int peer_fd) 
{
    while (1) {
        write(peer_fd , msg , strlen(msg));
        sleep(1);
    }




}
int main (int argc , char* argv[]) 
{
    ssize_t data_size = 0;
    /*Signal handling*/
    signal(SIGINT , signal_handler);

    struct addrinfo addr_info , *res;
    struct sockaddr client_info;
    struct sockaddr_in* ipv4_client_addr = (struct sockaddr_in*)&client_info;
    memset(&addr_info , 0 , sizeof(addr_info));
    openlog("writer" , LOG_CONS | LOG_PID , LOG_USER);
    syslog(LOG_USER , "This is an assignment5");

    sock_fd = socket(AF_INET , SOCK_STREAM , 0);

    if (sock_fd == -1) {
        ERROR_LOG("error ocurred while creating socket!\n");
        closelog();
        exit(EXIT_FAILURE);
    }

    addr_info.ai_family = AF_UNSPEC;
    addr_info.ai_socktype = SOCK_STREAM;
    addr_info.ai_flags = AI_PASSIVE;

    s = getaddrinfo(NULL , "9999" , &addr_info , &res);
    if (s != 0) {
        ERROR_LOG("error in getaddrinfo\n");
        closelog();
        exit(EXIT_FAILURE);
    }

    if (bind(sock_fd , res->ai_addr , res->ai_addrlen) != 0) {
        ERROR_LOG("error in binding\n");
        closelog();
        exit(EXIT_FAILURE);

    }
    freeaddrinfo(res);

    listen(sock_fd , 5);

    socklen_t client_len = sizeof(struct sockaddr);

    while (1) {
        printf("waiting to accept connection!\n");
        conn_fd = accept(sock_fd , &client_info , &client_len);
        inet_ntop(AF_INET , &(ipv4_client_addr->sin_addr) , client_ip , sizeof(client_ip));
        USER_LOG("Accepted connection from: %s" , client_ip);
        printf("connection established!\n");

        data_size = recv(conn_fd , recv_buffer , sizeof(recv_buffer) , 0);
        char *newline_position = strchr(recv_buffer, '\n'); // Find the position of '\n'

        if (newline_position != NULL) {
            printf("Newline found at position: %ld\n", newline_position - recv_buffer);
        } else {
            printf("Newline not found in the message.\n");
        }
        printf("size: %ld \n data received: %s" , data_size , recv_buffer);




        // write(conn_fd , msg , strlen(msg));
        // close(conn_fd);
        // sleep(1);

    }










    return 0;
}