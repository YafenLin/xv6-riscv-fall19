#include "kernel/types.h"
#include "user/user.h"

int main(void) 
{
    int parent_fd[2], child_fd[2];
    pipe(parent_fd);
    pipe(child_fd);
    char* ping = "ping";
    char* pong = "pong";
    char* buf[4];

    //如果是子进程
    if (fork() == 0){
        close(parent_fd[1]);
        close(child_fd[0]);
        read(parent_fd[0],buf,sizeof(buf));
        printf("%d: received %s\n",getpid(),buf);
        write(child_fd[1],pong,sizeof(pong));
    } else{ //父进程
        close(parent_fd[0]);
        close(child_fd[1]);
        write(parent_fd[1],ping,sizeof(ping));
        read(child_fd[0],buf,sizeof(buf));
        printf("%d: received %s\n",getpid(),buf);
    }
    exit();


}
