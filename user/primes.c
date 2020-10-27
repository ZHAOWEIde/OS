#include "kernel/types.h"
#include "user/user.h"
void getPrime(int *primes,int num);

int main(int argc, char *argv[]) {
  int num = 34;
  int wait_to_prime[num];
  for( int i =0 ;i < num;i++){//初始化
      wait_to_prime[i] =  i+2;
  }

  getPrime(wait_to_prime,num);

  exit();
}

void getPrime(int *primes,int num){

    if(num == 1){
        printf("prime %d\n",*primes);
        return;
    }
    else{
        printf("prime %d\n",primes[0]);
        int a_pipe[2];
        pipe(a_pipe);
        
        int i;
        int temp;
        //筛出来给子线程
        for(i = 1;i<num;i++){
            temp = primes[i];
            if(temp % primes[0] !=0){//除不尽
                write(a_pipe[1],(char *)(&temp),4);//传数据
            }
        }
        close(a_pipe[1]);//写关闭
        int get_fork;
        if((get_fork = fork()) == 0){//子进程
            // printf("now id is %d\n",getpid());
            char buff[4];
            int count = 0;
            int child_prime[34];
            while(read(a_pipe[0],buff,4)!=0){//读出来的东西放在buff
                child_prime[count] = *((int *)buff);
                // printf("child_prime[%d] is %d\n",count,child_prime[count]);
                count ++;
            }
            close(a_pipe[0]);
            //printf("count is %d\n",count);

            getPrime(child_prime,count);
            exit();
        }
        else{
            //printf("get_fork is %d\n",get_fork);
            //先运行父进程
            close(a_pipe[0]);
        }
        
    }
    wait();
}