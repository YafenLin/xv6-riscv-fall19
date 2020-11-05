
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
 
#define R 0
#define W 1
 /*
    父进程负责写，子进程负责读
 */
int main(int argc, char *argv[])
{
  int numbers[100], cnt = 0, i;
  int fd[2];
  //numbers数组存2-35
  for (i = 2; i <= 35; i++) {
    numbers[cnt++] = i;
  }
  while (cnt > 0) {
    pipe(fd);

    //子进程里
    if (fork() == 0) {
      //prime是传进来的数
      //this_prime是传出去的数
      int prime, this_prime = 0;
      close(fd[W]);
      cnt = -1;
      // 读的时候，如果父进程还没写，就会阻塞，如果父进程把写端关闭了且里面没数据了，就会返回0，此时cnt=-1，跳出外面的while循环
      while (read(fd[R], &prime, sizeof(prime)) != 0) {
          // 设置当前进程代表的素数，即第一个读进来的数
        if (cnt == -1) {
          this_prime = prime;
          cnt = 0;
        } else {
            //此时cnt=0,把此子进程筛出来的素数写进numbers数组
          if (prime % this_prime != 0) numbers[cnt++] = prime;
        }
      }
      printf("prime %d\n",this_prime);
      close(fd[R]);
    } else {
      // 父进程/上一次的子进程
      close(fd[R]);
      for (i = 0; i < cnt; i++) {
        write(fd[W], &numbers[i], sizeof(numbers[0]));
      }
      close(fd[W]);
      wait(); //等待子进程结束
      break;
    }
  }
  exit();
}
