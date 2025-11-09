#include <stdio.h>
int main() {
  int flag = 0;
  int l = 0;
  printf("Inserisci la lunghezza della sequenza: ");
  scanf("%d", &l);
  int array[l];
  for (int i = 0; i < l; i++) {
    printf("Inserisci l'elemento %d-esimo", i);
    scanf("%d", &array[i]);
  }
  for (int i = 0; i < l; i++) {
    for (int j = 0; j < l; j++) {
      if (i != j && array[i] == array[j]) {
        flag = 1;
      }
    }
  }
  if (flag == 1) {
    printf("L'array presenta almeno un elemento ripetuto");
  }
}
