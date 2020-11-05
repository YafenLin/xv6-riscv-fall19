#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    int i,k,l;
    int j = 0,m = 0;
    char block[32];
    char buf[32];
    char *p = buf;
    char *lineSplit[32];
    // printf("------------------------\n");
    //     printf("argc:%d\n",argc);
    for(i = 1; i < argc; i++){
        lineSplit[j++] = argv[i];
    }
    // int cnt = 0;
    // printf("还没开始读，等待输入参数\n");
    //0号文件描述符是标准读入，比如输入aaa，k=4，block=“aaa”
    //每一次输入参数执行依次while里面得read,一次read会读这次输入得所有参数，比如hello 1 sd sjdh
    while( (k = read(0, block, sizeof(block))) > 0){
        // printf("开始读\n");
        // cnt++;
        // printf("------------------------\n");
        // printf("block%d: %s\n",cnt,block);
        // printf("k:%d\n",k);
        for(l = 0; l < k; l++){
            if(block[l] == '\n'){
                buf[m] = 0;
                m = 0;
                //把lineSplit和buf存的最后一个参数连接起来
                lineSplit[j++] = p;
                p = buf; //p重新指向buf，为下一次命令行参数输入做准备
                lineSplit[j] = 0;   //结束符
               
                j = argc - 1;//下一次命令行参数输入做准备,让lineSplit重新赋值
                if(fork() == 0){
                    //argv[1]是echo程序
                    exec(argv[1], lineSplit);
                    // printf("------------------------\n");
                }                
                else{
                    wait();
                }
            }else if(block[l] == ' ') {
                buf[m++] = 0;
                // printf("buf:%s\n",buf);
                //把lineSplit和buf连接起来,lineSplit[argc]=第一个参数
                lineSplit[j++] = p;
                //p指向buf存的第二个参数开头
                p = &buf[m];
            }else {
                buf[m++] = block[l];
            }
        }
    }
    exit();
}
