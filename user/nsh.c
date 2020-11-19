/*
    参考代码：https://blog.csdn.net/RedemptionC/article/details/106962836?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522160523026619725222454306%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=160523026619725222454306&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-5-106962836.first_rank_ecpm_v3_pc_rank_v2&utm_term=Simple+xv6+shell&spm=1018.2118.3001.4449
*/
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int getcmd(char *buf, int nbuf);
void setargs(char *cmd, char* argv[],int* argc);
void runcmd(char*argv[],int argc);
void execPipe(char*argv[],int argc);

#define MAXARGS 10
#define MAXWORD 30
#define MAXLINE 100

//各种各样的空格
char whitespace[] = " \t\r\n\v";

 
 // 获取用户输入的命令
int getcmd(char *buf, int nbuf)
{
    //改变命令行输入提示符为@
    fprintf(2, "@ ");   
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0) // EOF
        return -1;
    return 0;
}

 // 设置参数
void setargs(char *cmd, char* argv[],int* argc)
{
    // 表示第i个word
    int i = 0; 
    // 用j来遍历cmd
    int j = 0;  

    // 每次循环找到参数
    // 让argv[i]分别指向参数的开头，并且将参数后面的空格设为\0，这样读取的时候可以直接读
    for (; cmd[j] != '\n' && cmd[j] != '\0'; j++)
    {
    
        //判断cmd[j]中有没有whitespace中的内容（即各种各样的空格），跳过
        while (strchr(whitespace,cmd[j])){
            j++;
        }
        argv[i++]=cmd+j;
        // 找到参数后面的空格
        while (strchr(whitespace,cmd[j])==0){
            j++;
        }
        // 把找到的参数后面的第一个空格设置为结束符
        cmd[j]='\0';
    }
    argv[i]=0;  //结束
    *argc=i;    //argc表示有几个参数
}
 
// 执行命令
void runcmd(char*argv[],int argc)
{
    for(int i=1;i<argc;i++){
        // 如果argv[i] == “|” 
        if(!strcmp(argv[i],"|")){
            // 支持两元素管道
            execPipe(argv,argc);
        }
    }

    //支持输入(<)输出(>)重定向
    for(int i=1;i<argc;i++){
        // 输出重定向 --- 关闭标准输出1
        if(!strcmp(argv[i],">")){
            close(1);
            // 这时候打开的文件的文件描述符是1，即这个文件是当前进程的输出
            open(argv[i+1],O_CREATE|O_WRONLY);
            // 输出重定向完毕，把符号>和它后面的东西截掉（把>变成0）
            argv[i]=0;
        }
        // 输入重定向 --- 关闭标准输入0
        if(!strcmp(argv[i],"<")){
            close(0);
            // 这时候打开的文件的文件描述符是0，即这个文件是当前进程的输入
            open(argv[i+1],O_RDONLY);
            // 输入重定向完毕，把符号<和它后面的东西截掉（把<变成0）
            argv[i]=0;
        }
    }
    // 执行程序且不返回
    exec(argv[0], argv);
}
 
void execPipe(char*argv[],int argc){
    int i=0;
    // 首先找到命令中的"|",然后把他换成'\0'
    // 从前到后，找到第一个就停止，后面都递归调用
    for(;i<argc;i++){
        if(!strcmp(argv[i],"|")){
            argv[i]=0;
            break;
        }
    }
    // cat file | wc 
    int fd[2];
    pipe(fd);
    if(fork()==0){
        close(1);
        dup(fd[1]);
        close(fd[0]);
        close(fd[1]);
        // 此时的子进程的标准输出（即文件描述符1）是管道的写入端fd[1]
        // 执行cat file
        runcmd(argv,i);
    }else{ 
        close(0);
        dup(fd[0]);
        close(fd[0]);
        close(fd[1]);
        // 此时的父进程的标准输入（即文件描述符0）是管道的读出端fd[0]
        // 执行 wc 
        runcmd(argv+i+1,argc-i-1);
    }
}
int main()
{
    // buf存储用户输入的命令
    char buf[MAXLINE];
    // 执行getcmd获得用户输入的命令
    while (getcmd(buf, sizeof(buf)) >= 0)
    {
        //创建子进程
        if (fork() == 0)
        {
            char* argv[MAXARGS];
            int argc=-1;
            // 重新设置参数格式
            setargs(buf, argv,&argc);
            //运行命令
            runcmd(argv,argc);
        }
        // 父进程等待子进程执行命令结束
        wait(0);
    }
 
    exit(0);
}