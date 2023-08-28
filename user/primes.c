#include "kernel/types.h"
#include "user/user.h"
//思路好想代码半个小时，但是debug问题出现在了pipe的索引上一个中间进程有2套pipe这块读写pipe
int
main(int argc, char *argv[]) {  
  int pipe1[36];
  int first_prime = -1;
  int read_int = 0;
  int pipe_index = 0;
  for(int i = 0; i < 36; i++) {
    pipe1[i] = -1;
  }
  pipe(pipe1);

  int pid = fork();
  if(pid > 0) {
    close(pipe1[0]);
    for(int i = 2; i <=35; i++) {
        if(first_prime == -1) {
            first_prime = i;
            printf("prime %d\n", first_prime);
        } else {
            if(i % first_prime != 0) {
              write(pipe1[1], &i, 4);
            }
        }
    }
    close(pipe1[1]);
  } else if(pid == 0) {
    while(read(pipe1[pipe_index], &read_int, 4)) {
        if(first_prime == -1) {
            close(pipe1[pipe_index+1]);
            first_prime = read_int;
            printf("prime %d\n", first_prime);
        } else {
            if(read_int % first_prime != 0) {
                if(pipe1[pipe_index + 2] == -1) {
                    pipe_index++;
                    pipe(pipe1+pipe_index);
                    if(fork() > 0) {
                      write(pipe1[pipe_index + 1 ], &read_int, 4);
                      pipe_index--;
                    } else{
                      first_prime = -1;
                    }
                } else {
                  write(pipe1[pipe_index + 2 ], &read_int, 4);
                }
            }
        }
    }
    close(pipe1[pipe_index]);
    close(pipe1[pipe_index+2]);
    wait(0);
    exit(0);
  } else {
    fprintf(2,"fork error\n");
  }
  wait(0);
  exit(0);
}
