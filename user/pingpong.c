#include "kernel/types.h"
#include "user/user.h"

int main(void) {
  int p_to_c[2];
  int c_to_p[2];
  char buf[8];

  if (pipe(p_to_c) < 0 || pipe(c_to_p) < 0) {
    fprintf(2, "pipe error\n");
    exit(1);
  }

  int pid = fork();
  if (pid == 0) {
    close(c_to_p[0]);
    close(p_to_c[1]);

    if (read(p_to_c[0], buf, 4) != 4) {
      fprintf(2, "read error\n");
      exit(1);
    }
    printf("%d: got %s\n", getpid(), buf);

    if (write(c_to_p[1], "pong", 4) != 4) {
      fprintf(2, "read error\n");
      exit(1);
    }

    close(c_to_p[1]);
    close(p_to_c[0]);
    exit(0);
  } else { // parent -- batyok
    close(c_to_p[1]);
    close(p_to_c[0]);

    if (write(p_to_c[1], "ping", 4) != 4) {
      fprintf(2, "parent write error\n");
      exit(1);
    }

    if (read(c_to_p[0], buf, 4) != 4) {
      fprintf(2, "parent read error\n");
      exit(1);
    }
    printf("%d: got %s\n", getpid(), buf);

    close(c_to_p[0]);
    close(p_to_c[1]);
    wait(0);
    exit(0);
  }
}