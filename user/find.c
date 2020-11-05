#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

/*
	将路径格式化为文件名
*/
char* fmt_name(char *path){
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--);
  p++;
  memmove(buf, p, strlen(p)+1);	//从p复制strlen(p)+1个字符到buf
//   printf("this:%s\n",buf);
  return buf;
}
/*
	系统文件名与要查找的文件名，若一致，打印系统文件完整路径
*/
void eq_print(char *fileName, char *findName){
	if(strcmp(fmt_name(fileName), findName) == 0){
		printf("%s\n", fileName);
	}
}

/*
	在某路径中查找某文件
*/
void find(char *path, char *findName){
	int fd;
	char buf[512], *p;
	struct stat st;		
	struct dirent de;
	// printf("myPath：%s\n",path);
	// printf("myFindName：%s\n",findName);

	//fd是文件描述符
	if((fd = open(path, O_RDONLY)) < 0){
		fprintf(2, "find: cannot open %s\n", path);
		return;
	}

	//如果无法获取信息
	if(fstat(fd, &st) < 0){
		fprintf(2, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}

	// printf("type:%d\n\n",st.type);
	//判断pd的类型
	switch(st.type){	
		case T_FILE:
			eq_print(path, findName);			
			break;
		case T_DIR:
			//如果目录名称过长
			if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
				printf("find: path too long\n");
				break;
			}
			strcpy(buf, path);
			//让p指向(buf数组即path)的尾部，因为是目录，所以加斜杠'/'
			p = buf+strlen(buf);
			*p++ = '/';
			
			//循环read()到struct dirent结构，得到其目录名/子文件，拼接得到当前路径后进入递归调用。
			while(read(fd, &de, sizeof(de)) == sizeof(de)){
				//不递归到'.'和'..'
				//inum是节点，name是文件/目录名
				if(de.inum == 0 || de.inum == 1 || strcmp(de.name, ".")==0 || strcmp(de.name, "..")==0)
					continue;			
				//复制de.name到p	
				memmove(p, de.name, strlen(de.name));
				//表示路径结束
				p[strlen(de.name)] = 0;
				//此时buf是拼接后得到的路径
				find(buf, findName);
			}
			break;
	}
	close(fd);	
}

int main(int argc, char *argv[]){
	if(argc < 3){
		printf("find: find <path> <fileName>\n");
		exit();
	}
	find(argv[1], argv[2]);
	exit();
}
