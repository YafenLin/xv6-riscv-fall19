#include "kernel/types.h"
#include "user/user.h"

//argv[0]指向输入的程序路径及名称, argv[1]指向输入的参数字符串。
int main(int argc, char *argv[])
{
  if(argc != 2) {
    fprintf(2, "must 1 argument for sleep\n");
    exit();
  }
  printf("(nothing happens for a little while)\n");
  sleep(atoi(argv[1]));
  exit();
}
