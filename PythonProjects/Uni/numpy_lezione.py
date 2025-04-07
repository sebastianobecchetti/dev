import numpy as np

# ruff: noqa: F001
# creo unarray_from_list lista e posso metterla dentro un array di numpy

# funzione per visualizzare meglio l'output


def print_with_space(text):
    print_with_space.count += 1
    print(f"ESERCIZIO NUMERO:  {print_with_space.count} volte")
    print(text, "\n\n\n")


print_with_space.count = 0
list1 = [1, 2, 3, 4]
array_from_list = np.array(list1)
print_with_space(array_from_list)

# ecco un esempio di un array di zeri con dimensioni (3,2,2)
array_of_zeros = np.zeros(shape=(3, 2, 2))
print_with_space(array_of_zeros)


# ecco un esempio di un array di inizializzato con degli uno e che ha dimensioni (4,1,1)
array_of_ones = np.ones(shape=(4, 1, 1))
print_with_space(array_of_ones)


# ecco un esempio di una matrice diagonale creata a partire da arra_of_ones
array_of_ones = np.ones(shape=(2, 2))
array_diag = np.diag(array_of_ones)
print_with_space(str(array_diag))
# nota bene la matrice diagonale viene visualizzata comunque come un vettore linea


# creiamo una matrice vuota di dimensione (3,3)
empty_matrix = np.empty(shape=(3, 3))
print_with_space(str(empty_matrix))


# creiamo una matrice identita'
identity_matrix = np.identity(n=3)
print_with_space(identity_matrix)


# esempi di come usare np.arange e linspace
# differenze: la prima ha (start,stop,step) la seconda ha (start,stop,"quante divioni fare")
arange_example = np.arange(
    0, 10, 1
)  # quindi arange fa delle divisioni in base allo step tipo ciclo for
linspace_example = np.linspace(
    0, 100, 10
)  # mentre lispace ha come terzo argomento il numero degli elementi da

print_with_space(arange_example)
print_with_space(linspace_example)

# un qualsiasi np.array ha come funzione .shape e .dtype il primo restituisce la dimensione del tensore mentre il secondo
# ne restituisce il type degli elementi dentro l'array
# .size invece darà come output il numero totale degli elementi
# .size si può usare anche su una sotto matrice con un esempio si capisce meglio:
a = np.ones(shape=(3, 3, 3))
print(a.size)  # da come output 27 perche' e' il numero totale degli elementi
print(
    a[0].size
)  # da come output 9 perche' e' il numero totale degli elementi della prima matrice (3,3)


# vediamo come funziona reshape con una tensore a una dimensione con  9 elementi che diventa un tensore (3,3)
a = np.arange(
    0, 9, 1
)  # volendo possiamo omettere il primo e l'ultimo argomento perche' vengono sotto'intesi
print(a, "\n\n")
print(a.reshape(3, 3), "\n\n")
# nota bene reshape funziona solo se è possibile effettivamente convertire in quelle dimensioni
