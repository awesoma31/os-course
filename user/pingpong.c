#include "kernel/types.h"
#include "user/user.h"

int main(void) {
  int pipka[2]; // pipe
  char buf[8];

  if (pipe(pipka) < 0) {
    fprintf(2, "pipe error\n");
    exit(1);
  }

  int pid = fork();
  if (pid == 0) { // ch
    if (read(pipka[0], buf, 4) != 4) {
      fprintf(2, "read error\n");
      exit(1);
    }
    printf("%d: got %s\n", getpid(), buf);

    if (write(pipka[1], "pong", 4) != 4) {
      fprintf(2, "read error\n");
      exit(1);
    }

    close(pipka[1]);
    exit(0);
  } else { // parent -- batyok
    if (write(pipka[1], "ping", 4) != 4) {
      fprintf(2, "parent write error\n");
      exit(1);
    }

    wait(0);  
    
    if (read(pipka[0], buf, 4) != 4) {
      fprintf(2, "parent read error\n");
      exit(1);
    }
    printf("%d: got %s\n", getpid(), buf);

    close(pipka[0]);
    exit(0);
  }
}