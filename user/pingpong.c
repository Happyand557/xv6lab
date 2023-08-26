#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {
  int pipe1[2];         //parent write child read
  int pipe2[2];         //parent read child write
  char buf;  
  int read_bytes;
  int write_bytes;

  pipe(pipe1);
  pipe(pipe2);
  int pid = fork();

  if(pid == 0) {
    buf = '0';
    close(pipe1[1]);
    read_bytes = read(pipe1[0], &buf, 1);
    if(!read_bytes) {
        fprintf(2, "child read fail\n");
        exit(-1);
    }
    close(pipe1[0]);

    printf("%d: received ping\n", getpid());

    close(pipe2[0]);
    write_bytes = write(pipe2[1], &buf, 1);
    if(!write_bytes) {
        fprintf(2, "child write fail\n");
        exit(-1);
    }
    close(pipe2[1]);
  } else if(pid != 0) {
    buf = '0';
    close(pipe1[0]);
    write_bytes = write(pipe1[1], &buf, 1);
    if(!write_bytes) {
        fprintf(2, "parents write fail\n");
        exit(-1);
    }
    close(pipe1[1]);

    close(pipe2[1]);
    read_bytes = read(pipe2[0], &buf, 1);
    if(!read_bytes) {
      fprintf(2, "parent read fail\n");
      exit(-1);
    }
    close(pipe2[0]);
    printf("%d: received pong\n", getpid());
  } else {
    fprintf(2, "fork error: pid < 0\n");
  }
  wait(0);
  exit(0);
}
