#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ARG_COUNT 3

int main (int argc , char* argv[]) 
{
    openlog("writer" , LOG_CONS | LOG_PID , LOG_USER);
    syslog(LOG_USER , "This is an assignment2");

    if (argc < MAX_ARG_COUNT) {
        syslog(LOG_ERR , "Too few arguments\n");
        closelog();
        exit(1);
    }
    else {

        FILE* file = fopen(argv[1] , "w");
        if (file != NULL) {
            fprintf(file , "%s\n" , argv[2]);
            fclose(file);

            syslog(LOG_DEBUG , "Writing to file %s with string %s\n." , argv[1] , argv[2]);

        }
        else {
            syslog(LOG_ERR , "File does not exist\n");
        }
    }
    

    closelog();
    return 0;
}