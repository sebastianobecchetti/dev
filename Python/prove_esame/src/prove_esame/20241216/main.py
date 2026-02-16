import numpy as np
import scipy.signal as sgn
import matplotlib.pyplot as plt


class DinamicSystem:
    def __init__(self, alpha):
        A = np.array([[-6, -alpha, 3], [1, 0, 0], [0, 1, 0]])
        B = np.array([[1, 0, 0]]).T
        C = np.array([0, 2, -1])
        D = np.array([[0]])

        self.__A = A * 2 + np.diag([-2, -2, -2])
        self.__B = B * np.array([[1, -0.5, 0.5]]).T
        self.__C = C + 1
        self.__D = D * -1 + 0.5
        self.__system = sgn.StateSpace(self.__A, self.__B, self.__C, self.__D)

    def print_system_info(self):
        print("A: ", self.__A)
        print("B: ", self.__B)
        print("C: ", self.__C)
        print("D: ", self.__D)
        poles = self.__system.poles
        print("poles: ", poles)
        print("ft: ", self.__system.to_tf)

    def bode_plot(self):
        w, mag, phase = sgn.bode(self.__system)
        fig, ax = plt.subplots(2)
        ax[0].semilogx(w, mag)
        ax[0].grid(True, which="both")
        ax[1].semilogx(w, phase)
        ax[1].grid(True, which="both")
        plt.show()

    def impulse_response(self):
        t, y = sgn.impulse(self.__system)
        fig, ax = plt.subplots(1)
        ax.plot(t, y)
        plt.show()

    def modulate_system(self):
        A_mod = np.random.normal(size=(3, 3), loc=0, scale=2)
        system = sgn.StateSpace(A_mod, self.__B, self.__C, self.__D)
        poles = system.poles
        if any(poles.real > 0):
            print("INSTABILE")
        else:
            print("STABILE")
        for p in poles:
            if p.real > 0:
                print("INSTABILE")
                return

        print("STABILE")

    @staticmethod
    def my_func():
        alpha = np.linspace(-50, 150, 5000)
        alpha_unstable = []
        alpha_stable = []
        for a in alpha:
            if any(DinamicSystem(a).__system.poles.real > 0):
                alpha_unstable.append(a)
            if len(alpha_unstable) == 3:
                break

        print("INSTABILI ALFA", alpha_unstable)


system = DinamicSystem(11)
system.bode_plot()
system.impulse_response()
system.modulate_system()
DinamicSystem.my_func()
