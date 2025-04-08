from os import supports_effective_ids
import pandas as pd
import seaborn as sea
import matplotlib

matplotlib.use("TkAgg")
import matplotlib.pyplot as plt

# #r = read, w = write, a = append
# with open("test.csv", "r") as f:
#     print(type(f))
#     # print(f.read()) e' commentato perche' una volta completato il passaggio sul file non posso ripassarci sopra
#     # di conseguenza print("riga\n", r) non verrebbe eseguito
#     row = f.readlines()
#     for r in row:
#         print("riga\n", r)
#     f.close()
#
#     with open("test.csv", "w") as f:
#         a = 3
#         b = 4
#         c = a + b
#         f.write(str(c))
#         f.close()
#
#


# riprndiamo il dataset gia' usato iris e facciamo un pò di operazioni con pandas
df = pd.read_csv("iris.data")
# print(df.head(10))  # stampa le prime 10 righe
#
# print(df.tail(10))  # stampa le ultime 10 righe
df = df[df["species"] == "setosa"]
print(df)
sea.pairplot(df)
plt.show()


s = pd.Series(["a", "b", "c", "d"], index=[5, 7, 11, 14])
print(s)


##############################
month = ["gennaio", "febbraio", "marzo"]
days = [31, 28, 31]
s = pd.Series(days, index=month)
print(s)

s = pd.Series(days, index=month)
# s1 = s.iloc[:3]
# s2 = s.iloc[2:]
# print(s1)
# print(s2)
# s_sum = s1 + s2
# print(s_sum)
print(s.describe())


def my_func(x):
    print("ELemento della serie:", x)
    return x**2


s = s.apply(my_func)
# se volessi dare altri argomenti (nel caso la funzione lo richiede) bast usare
# come secondo parametro di .apply() bisogna usare args = (n_1,n_2,...,n_n)

print(s)
# per convertire la series a numpy posso fare to_numpy(), oppure usando le funzionu specifiche delle series


# DataFrame
# Index = identifica le righe
# Columns = contiene le etichette (nomi) delle colonne del dataframe
# Shape = che descive le dimensioni del dataframe
# lo possiamo creare facendo un dizionario di Series per esempio

columns = {
    "Jobs": pd.Series(["engineer", "doctor"], index=["first", "second"]),
    "Cities": pd.Series(["Paris", "Rome"], index=["first", "second"]),
}
df = pd.DataFrame(columns)
print("\n\n\n", df)

df = pd.read_csv("iris.data")
df_filtered = df[df.species == "setosa"], ["sepal_lenght"]
print(df_filtered)

corr = df.corr(numeric_only=True)
sea.heatmap(corr, annot=True)
plt.show()
 
