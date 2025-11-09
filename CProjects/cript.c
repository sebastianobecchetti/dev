#include <stdio.h>

// --- FUNZIONI MATEMATICHE DI BASE ---

// Funzione per calcolare il Massimo Comune Divisore (MCD) usando l'algoritmo di
// Euclide
int gcd(int a, int b) {
  while (b != 0) {
    int temp = b;
    b = a % b;
    a = temp;
  }
  return a;
}

// Funzione per controllare se due numeri sono coprimi (MCD = 1)
int iscoprime(int n1, int n2) {
  return gcd(n1, n2) == 1; // Restituisce 1 se coprimi, 0 altrimenti
}

// Funzione per calcolare l'esponente privato 'd'
// Cerca d tale che (d * e) % z == 1
int calculate_d(int e, int z) {
  // Si cerca d nell'intervallo 1 < d < z.
  for (int k = 1; k < z; k++) {
    // Uso 'long long' per prevenire l'overflow nel prodotto k * e
    if (((long long)k * e) % z == 1) {
      return k;
    }
  }
  return 0; // Se non trovato
}

// Funzione per l'Elevamento a Potenza Modulare (Essenziale per RSA)
// Calcola (base^exp) % mod in modo efficiente e sicuro contro l'overflow.
long long power_mod(long long base, long long exp, long long mod) {
  long long res = 1;
  base %= mod;

  // Algoritmo di esponenziazione binaria (o 'square-and-multiply')
  while (exp > 0) {
    // Se l'esponente è dispari, moltiplica il risultato per la base
    if (exp % 2 == 1) {
      res = (res * base) % mod;
    }
    // L'esponente è dimezzato
    exp = exp >> 1;
    // La base è elevata al quadrato
    base = (base * base) % mod;
  }
  return res;
}

// --- FUNZIONE PRINCIPALE (MAIN) ---

int main() {
  int p, q;

  printf("--- RSA Key Generation & Crypto ---\n");
  printf(
      "Inserisci due numeri primi p e q (piccoli per il test, es: 17 e 11):\n");

  if (scanf("%d %d", &p, &q) != 2 || p <= 1 || q <= 1) {
    printf("Errore di input. Assicurati di inserire due numeri validi.\n");
    return 1;
  }

  // 1. Calcola il modulo n (Chiave pubblica)
  int n = p * q;

  // 2. Calcola la funzione Totiente di Eulero, z = phi(n)
  int z = (p - 1) * (q - 1);

  // 3. Trova l'esponente pubblico 'e' (Parte della chiave pubblica)
  int e = 0;
  for (int i = 2; i < z; i++) {
    if (iscoprime(i, z) == 1) {
      e = i;
      break; // Trovato il primo e valido
    }
  }

  if (e == 0) {
    printf(
        "Impossibile trovare un e valido. Controlla che p e q siano primi.\n");
    return 1;
  }

  // 4. Calcola l'esponente privato 'd' (Chiave privata)
  int d = calculate_d(e, z);

  if (d == 0) {
    printf("Errore: Impossibile trovare d.\n");
    return 1;
  }

  printf("\n--- Risultati Generazione Chiave ---\n");
  printf("Modulo n = p * q = %d\n", n);
  printf("Totiente z = (p-1)(q-1) = %d\n", z);
  printf("Esponente pubblico (e) = %d\n", e);
  printf("Esponente privato (d) = %d\n", d);
  printf("\nCHIAVE PUBBLICA: (e=%d, n=%d)\n", e, n);
  printf("CHIAVE PRIVATA: (d=%d, n=%d)\n", d, n);

  // --- CRITTOGRAFIA E DECRITTOGRAFIA ---

  int m;
  printf("\n--- Criptazione e Decriptazione ---\n");
  printf("Inserisci il messaggio (un numero intero positivo < %d):\n", n);

  // Il messaggio m deve essere minore di n per la corretta decrittografia RSA
  if (scanf("%d", &m) != 1 || m < 0 || m >= n) {
    printf("Input non valido o messaggio troppo grande.\n");
    return 1;
  }

  // 5. Criptazione: c = m^e mod n
  // Tutte le variabili sono promosse a long long per power_mod per sicurezza
  long long c = power_mod((long long)m, (long long)e, (long long)n);
  printf("\nMessaggio originale (m) = %d\n", m);
  printf("Messaggio criptato (c) = %lld\n", c);

  // 6. Decrittazione: m_decrittato = c^d mod n
  long long decrypted_m = power_mod(c, (long long)d, (long long)n);
  printf("Messaggio decrittato = %lld\n", decrypted_m);

  return 0;
}
