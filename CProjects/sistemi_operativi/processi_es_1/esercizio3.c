#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
int main(void) {
  // stampa immediata: aiuta a leggere l'output in tempo reale
  setvbuf(stdout, NULL, _IONBF, 0);
  int x = 10;
  printf("INIZIO ESERCIZIO\n");
  fflush(stdout);
  printf("x=%d\n", x);
  fflush(stdout);
  pid_t n = fork();
  x += 100;
  printf("Dopo la fork\n"); // dopo la fork il codice sottostante viene eseguito
                            // sia da padre che dal figlio
  printf("x=%d\n", x);
  if (n < 0) {
    perror("fork fallita");
    exit(1);
  } else if (n == 0) {
    // processo figlio
    printf("(figlio) fork=%d pid=%d ppid=%d\n", n, getpid(), getppid());
    printf("(figlio) per me x vale %d\n", x);
    x += (int)(getpid() / 100);
    printf("(figlio) Ora per me x vale %d\n", x);
    // exit(0); //se si toglie il commento il processo figlio esce appena
    // completa l'if e di conseguenza non stampa le ultime du righe di codice
  } else {
    printf("(padre) fork=%d pid=%d ppid=%d\n", n, getpid(), getppid());
    printf("(padre) per me x vale %d\n", x);
    // wait(NULL);
    x += getpid();
    printf("(padre) Ora per me x vale %d\n", x);
  }
  printf("Sono il processo %d e la mia variabile x vale %d\n", getpid(), x);
  printf("FINE ESERCIZIO\n");
  return 0;
}
