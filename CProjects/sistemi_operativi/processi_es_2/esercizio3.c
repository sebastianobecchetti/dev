#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
int main() {
  int p;
  while (1) {
    p = fork();
    if (!p) {
      printf("\nfiglio %d in esecuzione (mio padre: %d)\n", getpid(),
             getppid());
      exit(0);
    } else {
      sleep(2);
      // p=wait(0);
    }
  }
}
