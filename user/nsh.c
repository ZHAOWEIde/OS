#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXARGS 4

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

void getargs(char *cmd, char* argv[],int *argc);
void runcmd(char* argv[],int argc);
void usePipe(char *argv[],int argc,int index);
int getcmd(char *buf, int nbuf);

int main(void)
{
  static char buf[128];
  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(fork() == 0){
      char *argv[MAXARGS]; //参数列表
      int argc = -1;       //参数个数
      getargs(buf,argv,&argc); //处理缓冲区 得到命令 
      runcmd(argv,argc);
    }
    wait(0);
  }
  exit(0);
}

int getcmd(char *buf, int nbuf) //读取指令
{
  fprintf(2, "@ ");
  memset(buf, 0, nbuf); //初始话buf全为0
  gets(buf, nbuf);      //从缓冲区读
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

void getargs(char *buf, char* argv[],int *argc){
    int argv_num=0;  //参数个数
    int buf_index=0; //缓冲区索引
    for(;buf[buf_index]!='\n'&&buf[buf_index]!='\0';buf_index++){
        while(strchr(whitespace,buf[buf_index])){ //跳过空格
            buf_index++;
        }
        // echo  "hello world"
        argv[argv_num] = buf + buf_index; //参数列表存储第一个参数
        argv_num ++;
        while(strchr(whitespace,buf[buf_index]) == 0){ //寻找下一个空格
            buf_index++;
        }
        buf[buf_index]='\0';//修改成字符串终结符'\0'
    }
    argv[argv_num] = 0;
    *argc = argv_num;
}

void runcmd(char* argv[],int argc){

    for(int i = 0;i<argc;i++){
        if(!strcmp(argv[i],"|")){ //管道通信
            if(argc != i){ //后面至少还有一个参数
                usePipe(argv,argc,i);
                
            }
            else{
                fprintf(2,"%s\n","no argv after |");
            }
        }
        if(!strcmp(argv[i],">")){ //输出重定向到下一个文件！
            if(argc != i){ //后面至少还有一个参数
                close(1);
                open(argv[i+1],O_CREATE|O_WRONLY);
                argv[i] = 0; //停止
            }
            else{
                fprintf(2,"%s\n","no argv after >");
            }
        }
        if(!strcmp(argv[i],"<")){ //输入重定向
            if(argc != i){ //后面至少还有一个参数
                close(0);
                open(argv[i+1],O_RDONLY);
                argv[i] = 0; //停止
            }
            else{
                fprintf(2,"%s\n","no argv after <");
            }
        }
    }
    exec(argv[0],argv);
}


void usePipe(char *argv[],int argc,int index){
    argv[index] = 0;
    int p[2];
    pipe(p);
    if(fork() == 0){
        close(1);//关闭标准输出，执行左边的命令
        dup(p[1]);
        close(p[0]);
        close(p[1]);
        runcmd(argv,index);
    }
    else
    {
        close(0);//关闭标准输入,执行右边的命令
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        runcmd(argv+index+1,argc-index-1);
    }
}
