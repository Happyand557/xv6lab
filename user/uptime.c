#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {
    if(argc > 1) {
      fprintf(2, "uptime:uptime require no arg\n");
    }
    printf("%d\n", uptime());
    exit(0);
}