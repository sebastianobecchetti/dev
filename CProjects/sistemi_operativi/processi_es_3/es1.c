#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
  pid_t id;
  id = fork();
  if (id == -1)
    perror("Creazione figlio main");
  else if (id == 0) {
    printf("\n Ciao sono %d , il figlio di %d \n", getpid(), getppid());
    printf("\n Ora eseguo un programma diverso facendo una exec\n");
    execl("/bin/ls", "ls", "-l", NULL);
    printf("\n Questo messaggio non dovrebbe essere visualizzato\n");
  } else {
    printf("\n Padre: ho PID = %d \n", getpid());
    printf("\n Padre: ho un figlio avente PID = %d \n", id);
  }
  printf("\nFine esercizio\n");
}
