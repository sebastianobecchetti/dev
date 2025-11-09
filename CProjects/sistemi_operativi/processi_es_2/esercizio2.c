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
      exit(0);
    }
  }
  printf("Ho generato tutti i figli, io sono %d e mio padre e\' %d \n\n",
         getpid(), getppid());
  /* aspettiamo la terminazione dei figli */
  for (i = 0; i < count; i++) {
    n = wait(0);
    printf("\npadre: e' terminato il figlio con pid %d e io sono %d\n", n,
           getpid());
  }
}
