import sys
import numpy as np
import scipy.signal as sgn
import matplotlib.pyplot as plt


class DinamicSystem:
    def __init__(self, alpha):
        self.alpha = alpha
        A = np.array([[self.alpha, -2.0, 1.0], [0.0, -2.0, -1.0], [3.0, 0.0, 0.0]])
        B = np.array([[0.0, 1.0, -1.0]]).T
        C = np.array([[1.0, -1.0, 2.0]])
        D = np.array([[0.0]])
        self.__C = (A @ np.array([[-2, -2, -2]]).T).T
        print(self.__C)

        A = A * 3
        A = A - np.eye(3) * 1.5
        A[:, -1] += A[:, 1]
        self.__A = A
        print(self.__A)

        B = B * -1
        B[0] = B[0] * -1

        self.__B = B
        print(self.__B)

        D = D * 2 + 1

        self.__D = D[0]
        print(self.__D)
        self.system = sgn.StateSpace(self.__A, self.__B, self.__C, self.__D)

    def return_poles(self):
        p = self.system.poles
        return p

    def bode_plot(self):
        w, mag, phase = sgn.bode(self.system)
        fig, ax = plt.subplots(2)
        ax[0].semilogx(w, mag)
        ax[1].semilogx(w, phase)
        plt.show()

    def plot_impulse_response(self):
        time, response = sgn.impulse(self.system)
        fig, ax = plt.subplots(1)
        ax.plot(time, response)
        plt.show()

    def methods1(self, iter):
        for i in range(iter):
            M = np.random.uniform(size=(3, 3), low=-1, high=1)
            A_mod = self.__A + M
            system_modulated = sgn.StateSpace(A_mod, self.__B, self.__C, self.__D)
            poles = system_modulated.poles
            if any(poles.real > 0):
                print("Il sistema è instabile!, ITER = ", i)
                continue
            else:
                print("Il sistema è stabile, ITER = ", i)


sistema = DinamicSystem(-15)
print("poli: ", sistema.return_poles())
sistema.bode_plot()
sistema.plot_impulse_response()
sistema.methods1(5)
