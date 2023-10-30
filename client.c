#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#define MAX_COMMAND_SIZE 1024

int p[2];
char result[5000];
int main(int argc, int argv[])
{
    fflush(stdout);

    pid_t pid;
    if (pipe(p) == -1)
    {
        perror("Pipe error");
    }
    pid = fork();
    if (pid == -1) // Creare proces copil
    {
        perror("Fork error");
    }
    else if (pid == 0) // In procesul copil asteptam un input o comanda care sa fie trimisa spre server mai departe. Raspun
    {
      
            
            char command[MAX_COMMAND_SIZE];
            printf("[CLIENT] Waiting for input...\n");
            int fd = open("client-to-server", O_WRONLY);
            if (fd == -1)
                perror("Eroare la open");
            fgets(command, sizeof(command), stdin);
            int l = strlen(command);
            command[l] = '\0';
            
            if (strcmp(command, "quit") == 0)
            {
                char *msg = "quit_proc";
                close(p[0]);
                write(p[1], msg, sizeof(msg));
                close(p[1]);
            }

            if (-1 == write(fd, command, sizeof(command)))
            {
                perror("Eroare la write");
                exit(EXIT_FAILURE);
            }
            else
            {

                close(fd);
            }

            int stcfd = open("server-to-client", O_RDONLY);
            read(stcfd, result, sizeof(result));
            printf("%s\n", result);
           
        
        signal(SIGKILL, 0);
    }
    else
    {
        wait(NULL);

        close(p[1]);
        char rsp[2];

        read(p[0], rsp, sizeof(rsp));
        if (strcmp(rsp, "quit_proc") == 0)
        {
            printf("Proces inchis corespunzator\n");
        }
      
        else
            exit(EXIT_FAILURE);

        close(p[0]);
    }
}