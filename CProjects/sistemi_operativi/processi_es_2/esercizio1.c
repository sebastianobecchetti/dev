#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
int main() {
  int n, i, count;
  printf("Inizia l'esercizio\n");
  printf("Sono il processo iniziale: %d\n", getpid());
  printf("Immetti il numero di figli che vuoi creare\n");
  scanf("%d", &count);
  for (i = 0; i < count; i++) {
    n = fork();
    if (n == 0) { /* processo figlio */
      printf("\nfiglio %d in esecuzione (mio padre: %d)\n", getpid(),
             getppid());
      fflush(stdout);
      // exit(0); //se tolgo exit(0) ogni figlio fa un altro figlio
    }
  }
  printf("Ho generato tutti i figli, io sono %d e mio padre e\' %d \n\n",
         getpid(), getppid());
}
