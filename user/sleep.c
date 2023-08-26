#include "kernel/types.h"
#include "user/user.h"



int
main(int argc, char *argv[]) {
  char *iter;

  if(argc == 1) {
    fprintf(2, "sleep argc can not be empty\n");
    exit(-1);
  } else if(argc > 2) {
    fprintf(2, "sleep given too much args\n");
    exit(-1);
  }
  
  for(iter = argv[1]; *iter != '\0'; iter++) {
    if(*iter > '9' || *iter < '0') {
        fprintf(2, "%s is not a valid arg\n", argv[1]);
        exit(-1);
    }
  }

  sleep(atoi(argv[1]));
  exit(0);
}