#include <stdio.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>


int main (int argc , char* argv[]) 
{
    int sock_fd , s;
    struct addrinfo addr_info , *res;
    memset(&addr_info , 0 , sizeof(addr_info));
    openlog("writer" , LOG_CONS | LOG_PID , LOG_USER);
    syslog(LOG_USER , "This is an assignment5");

    sock_fd = socket(SOCK_STREAM , AF_INET , 0);

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
        syslog(LOG_ERR , "error in getaddrinfo\n");
        closelog();
        exit(EXIT_FAILURE);
    }

    if (bind(sock_fd , res->ai_addr , sizeof(res->ai_addr) != 0)) {
        syslog(LOG_ERR , "error in binding\n");
        closelog();
        exit(EXIT_FAILURE);

    }

    freeaddrinfo(res);











    return 0;
}