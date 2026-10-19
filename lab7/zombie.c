#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/* Creates a zombie child process on purpose */
int main(void)
{
    printf("parent: my PID is %d\n", getpid());
    fflush(stdout);

    pid_t pid = fork();                          /* step 1: copy myself */
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {                              /* only the child runs this */
        printf("child:  my PID is %d, my parent is %d\n", getpid(), getppid());
        fflush(stdout);
        execlp("ls", "ls", "-l", (char *)NULL);  /* step 2: become ls */
        perror("exec");
        exit(127);
    }

    int status;
    sleep(20);                                   /* parent sleeps while child dies -> zombie */
    waitpid(pid, &status, 0);                    /* step 3: wait reaps the zombie */
    printf("parent: child %d reaped with status %d\n", pid, WEXITSTATUS(status));
    return 0;
}
