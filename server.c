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
#include <utmp.h>
#include <time.h>
#define MAX_COMMAND_SIZE 1024
#define NAME_SIZE 100

int logged = 1;
int sp[2];
char result[5000];
char *login_modify(char *comm)
{
    char *name;
    memset(name, 0, strlen(comm) - 7);
    int i = 0;
    for (int j = 8; comm[j]; j++)
    {
        name[i++] = comm[j];
    }
    name[i] = '\0';
    return name;
}
void send_result_to_parent(const char *result)
{
    write(sp[1], result, strlen(result) + 1);
    close(sp[1]);
}
void handle_command(char *comm)
{
    printf("\n");
    if (comm[0] == 'l' && comm[1] == 'o' && comm[2] == 'g' && comm[3] == 'i' && comm[4] == 'n' && comm[5] == ' ' && comm[6] == ':' && comm[7] == ' ')
    {
        close(sp[0]);
        char name[MAX_COMMAND_SIZE];
        strcpy(name, login_modify(comm));

        FILE *file = fopen("users.txt", "r");

        if (file == NULL)
        {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        char line[NAME_SIZE];
        while (fgets(line, sizeof(line), file) != NULL)
        {
            int l = strlen(line);
            line[l - 1] = '\0';
            if (strcmp(name, line) == 0)
            {
                logged = 1;
                strcpy(result, "User logged!\n");

                break;
            }
        }
        if (logged == 0)
        {
            strcpy(result, "Couldn't login\n");
        }
    }
    else if (strcmp(comm, "get-logged-users") == 0)
    {

        struct utmp *entry;
        strcpy(result, " ");
        while (entry = getutent())
        {
            char *s = ctime((time_t *)&entry->ut_tv.tv_sec);
            char temp_res[5000];
            sprintf(temp_res, "%s : %s : %s\n", entry->ut_user, entry->ut_host, s);
            strcat(result, temp_res);
        }
    }
    else if (strcmp(comm, "logout") == 0)
    {
        if (logged == 0)
        {
            strcpy(result, "No user logged at the moment.\n");
        }
        else
        {
            logged = 0;
            strcpy(result, "Succesfully logged out!\n");
        }
    }
    else if (strstr(comm, "get-proc-info : "))
    {
        char pid[10];
        strcpy(pid, comm + 16);

        char fileName[1000] = "";
        strcpy(fileName, "/proc/");
        strcat(fileName, pid);
        strcat(fileName, "/status\0");
        int l = strlen(fileName);

        FILE *newFd;
        newFd = fopen(fileName, "r");

        char line[100];
        while (fgets(line, sizeof(line), newFd) != NULL)
        {

            strcat(result, line);
        }
    }
    send_result_to_parent(result);
}
int main(int argc, int argv[])
{
    int pid;

    fflush(stdout);
    printf("[SERVER] Started running...\n");
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == -1)
    {
        perror("Error at socket");
        exit(EXIT_FAILURE);
    }
    int cts, stc;

    pid = fork();
    if (pid == -1)
    {
        perror("Eroare la fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {

        if ((cts = mkfifo("client-to-server", 0777) == -1) || (stc = mkfifo("server-to-client", 0777) == -1))
        {
            if (errno == EEXIST)
            {
                perror("Already exists");
            }
            return -1;
        }

        int fd = open("client-to-server", O_RDONLY);
        if (fd == -1)
            exit(EXIT_FAILURE);
        printf("[SERVER] Waiting for a command...\n");
        char received[MAX_COMMAND_SIZE];
        read(fd, received, sizeof(received));

        if (strlen(received) != 0)
        {
            printf("[SERVER] Received a command!\n");
            int length = strlen(received);
            received[length - 1] = '\0';

            handle_command(received);
            printf("[SERVER] Handled the previous command.\n");
        }

        close(fd);
        signal(SIGKILL, 0);
    }
    else if (pid != 0)
    {

        while (1)
        {

            wait(NULL);
            close(sp[1]);
            char resp[5000];
            read(sp[0], resp, sizeof(resp));
            int len = strlen(resp);
            resp[len - 1] = '\0';
            // printf("%s\n",resp);
            int stcfd = open("server-to-client", O_WRONLY);
            write(stcfd, resp, sizeof(resp));
            close(stcfd);
            close(sp[0]);
            strcpy(result,"");
            pid=fork();
            //unlink("server-to-client");
            //unlink("client-to-server");
        }
    }

    return 0;
}