#include <stdio.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

#define DEBUG_LOG(msg,...) syslog(LOG_DEBUG,"SOCK_DEBUG: " msg "\n", ##__VA_ARGS__)
#define USER_LOG(msg,...) syslog(LOG_USER,"SOCK_DEBUG: " msg "\n", ##__VA_ARGS__)
#define ERROR_LOG(msg,...) syslog(LOG_ERR ,"SOCK_ERROR: " msg "\n" , ##__VA_ARGS__)

#define MAX_BUFFER_LEN 1024

char msg[200] = "connected to the server\n";
char client_ip[INET_ADDRSTRLEN] = {0};
// char recv_buffer[MAX_BUFFER_LEN] = {0};
int sock_fd , s , conn_fd , file_fd;
int total_data = 0;

char* file_path = "/var/tmp/aesdsocketdata";


static void signal_handler() 
{
    printf("server shutdown!\n");
    close(conn_fd);
    close(file_fd);
    // free(recv_buffer);
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
    char* recv_buffer = (char*)malloc(sizeof(char) * MAX_BUFFER_LEN);
    memset(recv_buffer , 0 , sizeof(recv_buffer));

    ssize_t data_size = 0;
    /*Signal handling*/
    signal(SIGINT , signal_handler);

    /*socket structures*/
    struct addrinfo addr_info , *res;
    struct sockaddr client_info;
    struct sockaddr_in* ipv4_client_addr = (struct sockaddr_in*)&client_info;
    memset(&addr_info , 0 , sizeof(addr_info));

    /*Opening necessery file*/
    openlog("writer" , LOG_CONS | LOG_PID , LOG_USER);
    syslog(LOG_USER , "This is an assignment5");
    file_fd = open(file_path, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

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
     

        data_size = recv(conn_fd , recv_buffer , MAX_BUFFER_LEN , 0);
        total_data += data_size;

        write(file_fd , recv_buffer , data_size);

        char* new_line = strchr(recv_buffer , '\n');

        if (new_line != NULL) {
            memset(recv_buffer , 0 , sizeof(recv_buffer));
            lseek(file_fd , 0 , SEEK_SET);
            read(file_fd , recv_buffer , sizeof(recv_buffer));
            printf("buffered data: %s\n" , recv_buffer);
            // send(conn_fd , recv_buffer , sizeof(recv_buffer) , 0);
        }
        
        printf("size: %ld \n data received: %s" , data_size , recv_buffer);


    }










    return 0;
}