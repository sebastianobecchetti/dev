import numpy as np
import matplotlib.pyplot as plt


class System:
    def __init__(self, alpha, M1, M2):
        A = np.random.normal(size=(3, 3), loc=0, scale=alpha)
        B = np.random.normal(size=(3, 3), loc=0, scale=alpha)
        self.__A = M1 @ A
        self.__B = M2 @ B
        self.__alpha = alpha

    def system_info(self):
        print("A: ", self.__A)
        print("B: ", self.__B)
        print("alpha: ", self.__alpha)

    def create_U(self, k):
        U = np.zeros(shape=(3, k))
        r_array = np.arange(0, k)
        U[0, :] = r_array
        U[1, :] = np.pow(r_array, 2)
        U[2, :] = np.mod(r_array, 2)
        return U

    def response(self, k, x_0):
        U = self.create_U(k)
        X = np.zeros(shape=(3, k + 1))
        X[:, 0] = x_0  # riempe tutte le righe della colonna 0 con x_0
        for c in range(1, k):
            X[:, c] = self.__A @ X[:, c - 1] + self.__B @ U[:, c - 1]
        return X

    def plot_response(self, k, x_0):
        r = self.response(k, x_0)
        fig, ax = plt.subplots(3)
        ax[0].plot(range(r.shape[1]), r[0, :])
        ax[1].plot(range(r.shape[1]), r[1, :])
        ax[2].plot(range(r.shape[1]), r[2, :])
        plt.show()

    def custom_func(self, c):
        D = np.zeros(shape=(3, 3))
        for i in range(D.shape[0]):
            for j in range(D.shape[1]):
                if i == j:
                    D[i, j] = self.__A[i, j] + self.__B[i, j]
        D[D > c] += 3
        print("Matrice D: ", D)


M1 = np.array([[-0.2, -0.1, -0.3], [0.1, 0.0, 0.2], [0.0, 0.1, 0.1]])
M2 = np.array([[0.0, -0.1, -0.0], [0.1, 0.1, 0.1], [0.0, 0.1, 0.2]])
x_0 = np.array([[1.0, 1.0, 1.0]])
sistema = System(2, M1, M2)
sistema.system_info()
sistema.plot_response(50, x_0)
sistema.custom_func(0.0)

