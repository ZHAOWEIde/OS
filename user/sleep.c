#include "kernel/types.h"
#include "user/user.h"
//argc 命令行总的参数个数
//argv[]是argc个参数，其中第0个参数是程序的全名，
//以后的参数命令行后面跟的用户输入的参数，

int main(int argc, char *argv[]) {
  if (argc != 2)
    write(1, "Something wrong!", strlen("Something wrong!"));

// ssize_t write(int fd,const void*buf,size_t count);
// 参数说明：
// fd:是文件描述符（输出到command line，就是1）
// buf:通常是一个字符串，需要写入的字符串
// count：是每次写入的字节数

  int x = atoi(argv[1]);
  //atoi() 函数用来将字符串转换成整数(int)，

  sleep(x);

  exit();
}
