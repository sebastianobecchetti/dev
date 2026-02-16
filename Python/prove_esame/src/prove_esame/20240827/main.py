import numpy as np
import scipy.signal as sgn
import matplotlib.pyplot as plt


class DinamicSystem:
    def __init__(self, alpha):
        self.__A = np.array([[-8.0, -alpha, -8], [1, 0, 0], [0, 1, 0]])
        self.__B = np.array([[1.0, 0, 0]]).T
        self.__C = np.array([[0.0, 4, -3]])
        self.__D = np.array([[0]])
        self.__system = sgn.StateSpace(self.__A, self.__B, self.__C, self.__D)

    def desribe(self):
        poles = self.__system
        print("Poli del sistema: ", poles)
        print("TF: ", self.__system.to_tf)

    def plot_bode(self):
        fig, ax = plt.subplots(2)
        w, mag, phase = sgn.bode(self.__system)

        ax[0].semilogx(w, mag, color="red")
        ax[0].set_title("Bode Modulo")
        ax[1].semilogx(w, phase, color="blue")
        ax[1].set_title("Bode Fase")
        plt.show()

    def step_response(self):
        t_step, step_r = sgn.step(self.__system)
        fig, ax = plt.subplots(1)
        ax.plot(t_step, step_r, color="cyan")
        ax.set_title("Risposta al gradino")
        plt.show()


sistema_dinamico = DinamicSystem(13)
sistema_dinamico.desribe()
sistema_dinamico.plot_bode()
sistema_dinamico.step_response()
