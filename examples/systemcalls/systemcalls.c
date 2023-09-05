#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    // printf("cmd: %s" , cmd);
    if (system(cmd) == -1) {
        perror("Error in calling system call\n");
        return false;
    }
    return true;



/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/

}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    pid_t pid;
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];

    pid = fork();

    if ( pid == -1 ) {      // if case of error in forking
        fprintf(stderr , "Error: %d - %s" , errno , strerror(errno));
        return false;
    }
    else if ( pid > 0 ) {       // if pid is greater then 0 then it is a parent process.
        int status;
        printf("This is a parent process with child processID: %d\n" , pid);
        int return_code = waitpid(pid , &status , 0);
        if ( return_code == -1 ) {
            fprintf(stderr , "Error: %d - %s" , errno , strerror(errno));
            return false;
        }
        if (WEXITSTATUS(status) != 0) {
            printf("The child has died with exit code: %d\n" , WEXITSTATUS(status));
            return false;

        } 
        
    }
    else {      // if pid is equal to zero then it is child process.
        
        printf("This is a child process : %d\n" , pid);
        if (execv(command[0] , command) == -1) {
            // fprintf(stderr , "Error: %d - %s" , errno , strerror(errno));
            perror("Error in execv system call\n");
            exit(1);
            
        }
                
        
       
        
    }
    

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    
    pid_t pid;
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
        printf("command[%d] = %s\n" , i , command[i]);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count]; 

    pid = fork();


    if ( pid == -1) {
        fprintf(stderr , "Error: %d - %s" , errno , strerror(errno));
        return false;


    }
    else if ( pid > 0 ) {       // this is parent process
        int status;   
       
        printf("This is a parent process with child processID: %d\n" , pid);

        int return_code = waitpid(pid , &status , 0);
        if ( return_code == -1 ) {
            fprintf(stderr , "Error: %d - %s" , errno , strerror(errno));
            return false;
        }
        if (WEXITSTATUS(status) != 0) {
            printf("The child has completed the task with status code: %d\n" , WEXITSTATUS(status));
            return false;

        }




    }
    else {      // This is child process.


        int fd = open(outputfile , O_CREAT | O_RDWR , 00644);

        if ( fd == -1 ) {

            fprintf(stderr , "Error: %d - %s" , errno , strerror(errno));
            return false;
        } 


        printf("This should go to the standered out\n");

        if (dup2(fd , STDOUT_FILENO) == -1) {
            perror("Error in duplicating file discriptors\n");
            return false;
        }

        if (execv(command[0] , command) == -1) {
            perror("Error in execv\n");
            return false;
        }


        close(fd);


    }



/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    va_end(args);

    return true;
}
// #define REDIRECT_FILE "testfile.txt"

// int main () 
// {


//     // do_system("echo this is a test > " REDIRECT_FILE );
//     do_exec(2 , "/usr/bin/ls" , "-l");
//     return 0;
// }