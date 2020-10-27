#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int parent_to_child_pipe[2];
    int child_to_parent_pipe[2];
    pipe(parent_to_child_pipe);
    pipe(child_to_parent_pipe);
    char buf[64];
    if(fork() == 0){//child
        write(child_to_parent_pipe[1],"pong",strlen("pong"));
        read(parent_to_child_pipe[0],buf,4);
        printf("%d: received %s\n",getpid(),buf);
    }else{//parent
        write(parent_to_child_pipe[1],"ping",strlen("ping"));
        read(child_to_parent_pipe[0],buf,4);
        printf("%d: received %s\n",getpid(),buf);
    }

  exit();
}