import numpy as np
import pandas as pd
import seaborn as sea
import matplotlib

matplotlib.use("TkAgg")
import matplotlib.pyplot as plt

pd.set_option("display.expand_frame_repr", False)
df = pd.read_csv("titanic.data")
df["Has_Cabin"] = ~df.Cabin.isnull()
df = df.drop(["Cabin"], axis=1)
df = df.drop(["Ticket"], axis=1)
df = df.drop(["PassengerId"], axis=1)


def impute_age(cols):
    age = cols[0]
    pclass = cols[1]
    if pd.isnull(age):
        if pclass == 1:
            return 38
        elif pclass == 2:
            return 29
        else:
            return 24
    else:
        return age


df["Age"] = df[["Age", "Pclass"]].apply(impute_age, axis=1)
# sea.boxplot(x="Pclass", y="Age", data=df, palette="viridis")
# df = df.drop(["Age"], axis=1)
# print(df.describe())
df = df.dropna()
sea.heatmap(df.isnull(), cmap=("viridis"))
# print(df.head())
#
plt.show()
