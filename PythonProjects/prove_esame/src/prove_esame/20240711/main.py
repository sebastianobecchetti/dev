import numpy as np
import matplotlib.pyplot as plt
from scipy.fft import fft, fftfreq
import scipy.signal as sgn


class DinamicSystem:
    def __init__(self, A, frequency, L, durata, sampling_rate):
        self.__A = A
        self.__frequency = frequency
        self.__L = L
        self.__durata = durata
        self.__sampling_rate = sampling_rate

        self.__time_index = np.linspace(
            0, self.__durata, int(self.__durata * self.__sampling_rate)
        )
        self.signal = A * np.sin(2 * np.pi * self.__frequency * self.__time_index)
        for l in range(2, L + 1):
            self.signal += (
                l
                % 2
                * self.__A
                * np.sin(2 * np.pi * l**3 * self.__frequency * self.__time_index)
            )
            self.signal += (
                (l + 1)
                % 2
                * self.__A
                * np.cos(2 * np.pi * l**3 * self.__frequency * self.__time_index)
            )

    def fft_plot(self):
        fig, ax = plt.subplots(2)
        ax[0].plot(self.signal)

        signal_fft = fft(self.signal)
        xf = fftfreq(
            int(self.__sampling_rate * self.__durata), 1 / self.__sampling_rate
        )
        ax[1].stem(xf, np.abs(signal_fft))

        plt.show()

    def filter_signal(self, M, f_cutoff):
        filter = sgn.firwin(numtaps=M, cutoff=f_cutoff, fs=self.__sampling_rate)
        signal_filtered = sgn.convolve(self.signal, filter, mode="same")

        fig, ax = plt.subplots(4)
        ax[0].plot(self.signal)
        ax[0].set(title="segnale originale")
        w1, h1 = sgn.freqz(filter)
        ax[1].plot(w1, 20 * np.log10(np.abs(h1)))
        ax[1].set(title="Filtro")
        ax[2].plot(signal_filtered)
        ax[2].set(title="segnale filtrato")

        fft_signal_filtered = fft(signal_filtered)
        xf = fftfreq(
            int(self.__sampling_rate * self.__durata), 1 / self.__sampling_rate
        )
        ax[3].stem(xf, np.abs(fft_signal_filtered))
        ax[3].set(title="fft del segnale filtrato")

        plt.show()

        return signal_filtered

    def step_adder(self, s):
        step_signal = np.zeros(len(self.__time_index))
        step_signal[int(self.__durata * self.__sampling_rate // 2) :] = 10
        s += step_signal
        fig, ax = plt.subplots(1)
        ax.plot(s)
        plt.show()

    def custom_func(self):
        array = np.random.normal(size=(self.signal.shape[0] - 1, 1), loc=1, scale=3)
        results = self.signal * array
        results = results[results < 2]
        media = np.mean(results)
        print("media: ", media)
        print("risultato: ", results)


signal = DinamicSystem(2.0, 6.0, 4, 3.0, 2000)
signal.fft_plot()
signal.filter_signal(80, 8)
signal.step_adder(signal.signal)
signal.custom_func()
