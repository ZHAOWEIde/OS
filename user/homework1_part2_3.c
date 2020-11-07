#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

void sigint_handler(int sig) /* SIGINT handler */
{
    printf("received a signal\n");
    kill(getpid(),SIGKILL);
}

void main()
{
    int child_pid;
    if ((child_pid = fork()) == 0)
    {
        while (1)
            ;
        printf("forbidden zone\n");
        exit(0);
    }
    else
    {
        if (signal(SIGINT, sigint_handler) == SIG_ERR)
            unix_error("signal error");
        while (getc(stdin))
        {
            wait(0);
            exit(0);
        }
    }
}