#include <stdio.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

char msg[200] = "connected to the server\n";
int sock_fd , s , conn_fd;


static void signal_handler() 
{
    printf("server shutdown!\n");
    close(conn_fd);
    exit(EXIT_SUCCESS);



}
int main (int argc , char* argv[]) 
{
    // int sock_fd , s , conn_fd;

    /*Signal handling*/
    signal(SIGINT , signal_handler);
    struct addrinfo addr_info , *res;
    struct sockaddr client_info;
    memset(&addr_info , 0 , sizeof(addr_info));
    openlog("writer" , LOG_CONS | LOG_PID , LOG_USER);
    syslog(LOG_USER , "This is an assignment5");

    sock_fd = socket(AF_INET , SOCK_STREAM , 0);

    if (sock_fd == -1) {
        syslog(LOG_ERR , "error ocurred while creating socket!\n");
        closelog();
        exit(EXIT_FAILURE);
    }

    addr_info.ai_family = AF_UNSPEC;
    addr_info.ai_socktype = SOCK_STREAM;
    addr_info.ai_flags = AI_PASSIVE;

    s = getaddrinfo(NULL , "9999" , &addr_info , &res);
    if (s != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
            exit(EXIT_FAILURE);
        }


    if (s != 0) {
        syslog(LOG_ERR , "error in getaddrinfo\n");
        closelog();
        exit(EXIT_FAILURE);
    }

    if (bind(sock_fd , res->ai_addr , res->ai_addrlen) != 0) {
        syslog(LOG_ERR , "error in binding\n");
        closelog();
        exit(EXIT_FAILURE);

    }
    freeaddrinfo(res);

    listen(sock_fd , 5);

    socklen_t client_len = sizeof(struct sockaddr);

    while (1) {
        conn_fd = accept(sock_fd , &client_info , &client_len);
        write(conn_fd , msg , strlen(msg));
        
        close(conn_fd);
        sleep(1);

    }










    return 0;
}