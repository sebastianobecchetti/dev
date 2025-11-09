#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  setvbuf(stdout, NULL, _IONBF,
          0); // stampa immediata: aiuta a leggere l'output in tempo reale
  int n, pid;
  n = fork();
  if (n == -1) {
    fprintf(stderr, "fork fallita\n");
    exit(1);
  } else if (n == 0) /* processo figlio */
  {
    printf("\nsono il figlio; risultato della fork = %d\n", n);
    printf("\n(figlio) il mio process-id e` %d\n", getpid());
    printf("\n(figlio) il process-id di mio padre e` %d\n", getppid());
    exit(0);
  } else
  /* processo padre */
  {
    printf("\nsono il padre; risultato della fork = %d\n", n);
    printf("\n(padre) il mio process-id e` %d\n", getpid());
    printf("\n");
    pid = wait(NULL);
    printf("\n(padre) il process-id di mio padre e` %d\n", getppid());
    exit(0);
  }
}
