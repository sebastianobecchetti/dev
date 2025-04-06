# Scrivere un programma Python che elabora un intero di 6 cifre e visualizza le singole cifre di cui è
# composto. Ad esempio, avendo il numero 458932, il programma deve visualizzare, su righe separate 4 5
# 8932


def esercizio_1():
    numero_inserito = input("Inserisci un numero di sei cifre: ")
    while len(str(numero_inserito)) != 6:
        print("Hai inserito un input non valido")
        numero_inserito = input("Inserisci un numero di sei cifre: ")

    print(
        "il numero inserito e'", numero_inserito, "\n Il numero diviso in 6 cifre è: "
    )

    for c in numero_inserito:
        print(c)

    return 0


def esercizio_2():
    stringa = input("Inserisci la stringa: ")
    if any(char.isdigit() for char in stringa):
        print("La stringa contiene almeno una cifra")

    if stringa.isdigit():
        print("La stringa contiene solo cifre")

    if stringa.islower():
        print("La stringa contiene tutte lettere minuscole")
    else:
        print("La stringa NON contiene tutte lettere minuscole")

    if any(not char.isalnum() for char in stringa):
        print("La stringa contiene caratteri speciali")


# Scrivere un programma Python che chiede all’utente di inserire quattro numeri e mostra in console la
# parola “crescenti” se sono in ordine strettamente crescente, “decrescenti” se sono in ordine
# strettamente decrescente e “nessuno dei due” se non sono né in ordine crescente né in ordine
# decrescente.


def esercizio_3():
    list1 = [0, 0, 0, 0]
    for i in range(len(list1)):
        print("Inserisci l'elemento", i + 1)
        list1[i] = int(input())
    print("Ecco la lista che hai inserito", list1)
    sorted_list = sorted(list1)
    reversed_list = sorted_list[::-1]
    print("Questa invece è la lista che è stata ordinata e capovolta", reversed_list)
    if list1 == reversed_list:
        print("LA LISTA E' STRETTAMENTE DECRESCENTE")
    if list1 == sorted_list:
        print("LA LISTA È STRETTAMENTE CRESCENTE")


def esercizio_4():
    check = ""
    while True:
        check = input('Inserisci "couniugato" o "non coniugato"\n')
        if check == "coniugato":
            print("Hai scelto coniugato")
            check_coniugato()
            break
        elif check == "non coniugato":
            print("Hai scelto non coniugato")
            check_non_coniugato()
            break
        else:
            print("Hai scritto male o fatto la scelta sbagliata")


def check_coniugato():
    reddito = int(input("Inserisci il tuo reddito:\n"))
    if reddito <= 10000:
        print("Non devi dare nessuna tassa")
    elif reddito > 10000 and reddito <= 30000:
        taxes = (reddito - 10000) * 12 / 100
        print("Le tasse che devi sono: ", taxes)
    elif reddito > 30000:
        taxes = (reddito - 30000) * 30 / 100
        print("Le tasse che devi sono: ", taxes)


def check_non_coniugato():
    reddito = int(input("Inserisci il tuo reddito:\n"))
    if reddito <= 10000:
        print("Non devi dare nessuna tassa")
    elif reddito > 10000 and reddito <= 30000:
        taxes = (reddito - 10000) * 15 / 100
        print("Le tasse che devi sono: ", taxes)
    elif reddito > 30000:
        taxes = (reddito - 30000) * 35 / 100
        print("Le tasse che devi sono: ", taxes)


# Scrivere un programma Python che data una lista di numeri sostituisca ciascun valore con la media tra il
# valore stesso e i due adiacenti (o dell’unico valore adiacente se il valore in esame si trova all’estremità
# della lista). Scrivere due versioni del programma: una in cui i nuovi valori sono inseriti in una nuova lista
# e un’altra in cui i valori sono sostituiti direttamente nella lista di partenza.
def esercizio_5():
    list1 = []
    list2 = []
    n_element = int(input("Inserisci il numero di elementi: "))
    for i in range(n_element):
        print("Inserisci l'elemento ", i)
        list1.append(int(input()))
    print("Ecco la lista che hai inserito: ", list1)

    for i in range(n_element):
        if i == 0:
            list2.append((list1[i] + list1[i + 1]) / 2)
        elif i == n_element - 1:
            list2.append((list1[-1] + list1[n_element - 2]) / 2)
        else:
            list2.append((list1[i - 1] + list1[i] + list1[i + 1]) / 3)
    print("Ecco la lista con le medie:\n", list2)


while True:
    choice = input("Inserisci l'esercizio da eseguire: (0 per uscire):\n")
    if choice == "1":
        esercizio_1()
    elif choice == "2":
        esercizio_2()
    elif choice == "3":
        esercizio_3()
    elif choice == "4":
        esercizio_4()
    elif choice == "5a":
        esercizio_5()
    elif choice == "0":
        print("Programma terminato correttamente:")
        break
    else:
        print("#ERRORE# scelta non valida")
