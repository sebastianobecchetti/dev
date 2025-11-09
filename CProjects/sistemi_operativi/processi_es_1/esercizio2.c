#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0); // stampe immediate
  pid_t n = fork();
  if (n < 0) {
    perror("fork");
    exit(1);
  }
  const int N = 50; // ripetizioni per allungare un po' l'esecuzione
  if (n == 0) {     // FIGLIO
    for (int i = 0; i < N; ++i) {
      // mini-lavoro per consumare CPU
      volatile unsigned int w = 0;
      for (int k = 0; k < 200000; ++k)
        w++;
      printf("C%02d(pid=%d)\n", i, getpid());
    }
    printf("\n[FIGLIO termina]\n");
    exit(0);
  } else {

    // pid_t pid = wait(NULL); // posizioniamo la wait prima di tutto e vediamo
    // cosa succede PADRE
    for (int i = 0; i < N; ++i) {
      volatile unsigned int w = 0;
      for (int k = 0; k < 200000; ++k)
        w++;
      printf("P%02d(pid=%d)\n", i, getpid());
    }
    printf("\n");
    pid_t pid = wait(NULL); // raccoglie il figlio
    printf("[PADRE ha fatto wait()]\n");
  }
  return 0;
}
