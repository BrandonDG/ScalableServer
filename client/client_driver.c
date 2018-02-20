#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  pid_t  childpid = 0;
  size_t n;
  FILE   *fp;

  if (argc != 2) {
    fprintf(stderr, "[command] [exec_number]");
    return 1;
  }
  n = atoi(argv[1]);

  if ((fp = fopen("client_results", "w")) == 0) {
    fprintf(stderr, "client_driver fopen\n");
  }
  fclose(fp);

  for (size_t i = 0; i < n; i++) {
    if ((childpid = fork()) <= 0) {
      break;
    }
  }

  if (childpid == 0) {
    execl("/home/brandondg/Documents/COMP8005/A2/client/clt", "./clt", "localhost", (char *)0);
  }
  return 0;
}
