#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
  char bufRread[512];
  char buf[32][32];
  char *refs[32];
  for(int i=0; i<32;i++){
      refs[i] = buf[i];
  }

  for(int i = 1;i<argc; i++){
      strcpy(buf[i-1],argv[i]); //传参
  }
  int backNum; //读缓冲区的返回值;
  while((backNum = read(0,bufRread,sizeof(bufRread)))>0){
      int pos = argc - 1;
      char *ref = buf[pos];
      for(char *p = bufRread;*p;p++){
          if(*p == ' '||*p == '\n' ){
              *ref = '\0';
              pos ++;
              ref = buf[pos];
          }
          else{
              *ref++ = *p;
          }
         
      }
      *ref = '\0';
      pos++;
      refs[pos] = 0;
      if(fork()){//父进程
          wait(); // 等待子进程
      }
      else{
          exec(refs[0],refs);//子程序
      }
  }

  if(backNum<0){
      printf("You can't read!");
      exit();
  }
  exit();
}
