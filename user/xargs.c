#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"
//exec fucntion args need to include excutable program location
int
main(int argc, char *argv[]) {
  char byte;
  char *args[MAXARG];
  char buf[256]; //store one arg
  int index = 0; //buf index
  int args_index = 0;
  if(argc < 2) {
    fprintf(2, "xargs:no command input\n");
    exit(-1);
  }

  for(int i = 0; i < MAXARG; i++) {
    args[i] = 0;
  }

  while(read(0, &byte, 1)) {
    if(byte == '\n') {
      buf[index++] = '\0';
      index = 0;
      if(fork() == 0) {
        for(int i = 1; i < argc; i++) {
          args[args_index++] = argv[i];
        }
        args[args_index++] = buf;
        exec(argv[1], args);
        fprintf(2,"xargs:exec can't find %s\n", argv[1]);
        exit(0);
      } else {
        wait(0);
      }
    } else {
      buf[index++] = byte;
    }
  }
  exit(0);
}
