import numpy as np
import scipy.signal as sgn
import matplotlib

matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from scipy.fft import fft, fftfreq


class CustomSignal:
    def __init__(self, A, frequency, L, duration, sampling_rate):
        self.__frequency = frequency
        self.__A = A
        self.__L = L
        self.__duration = duration
        self.__sampling_rate = sampling_rate
        self.__time_index = np.linspace(
            0, self.__duration, int(self.__sampling_rate * self.__duration)
        )

        self.__signal = self.__A * np.sin(
            2 * np.pi * self.__frequency * self.__time_index
        )
        for l in range(2, self.__L + 1):
            print(4 * l**3)
            self.__signal += (
                (l % 2)
                * self.__A
                * np.sin(2 * np.pi * (l**3) * self.__frequency * self.__time_index)
            )
            self.__signal += (
                (l + 1 % 2)
                * self.__A
                * np.cos(2 * np.pi * (l**3) * self.__frequency * self.__time_index)
            )

    def plot_signal_and_fft(self):
        fft_sine = fft(self.__signal)
        xf = fftfreq(
            int(self.__duration * self.__sampling_rate), 1.0 / self.__sampling_rate
        )
        fig, ax = plt.subplots(2)
        ax[0].plot(self.__time_index, self.__signal)
        ax[0].set(title="Segnale", ylabel="s[n]", xlabel="Timestep n")
        ax[1].stem(xf, np.abs(fft_sine))
        ax[1].set(title="FFT del segnale", ylabel="abs(FFT)", xlabel="freq")
        plt.show()

    def convolve_and_plot(self, f_cutoff, M):
        filter = sgn.firwin(M, f_cutoff, fs=self.__sampling_rate)
        fig, ax = plt.subplots(4)

        ax[0].plot(self.__signal)
        ax[0].set(title="Segnale originale", xlabel="Timestep n")
        convolved_signal = sgn.convolve(self.__signal, filter, mode="same")

        w1, h1 = sgn.freqz(filter)
        ax[1].plot(w1, 20 * np.log10(np.abs(h1)))
        ax[1].set(title="Abs filtro (dB)", xlabel="Freq")
        ax[2].plot(convolved_signal)
        ax[2].set(title="Segnale filtrato", xlabel="Timestep n")
        fft_sine = fft(convolved_signal)
        xf = fftfreq(
            int(self.__duration * self.__sampling_rate), 1.0 / self.__sampling_rate
        )
        ax[3].stem(xf, np.abs(fft_sine))
        ax[3].set(title="FFT del segnale filtrato", ylabel="abs(FFT)", xlabel="freq")
        plt.show()
        return convolved_signal

    def add_step_signal(self, convolved_signal):
        step_signal = np.zeros(len(self.__time_index))
        step_signal[int(self.__duration * self.__sampling_rate // 2) :] = 10.0
        print(step_signal)
        convolved_signal += step_signal
        fig, ax = plt.subplots(1)
        ax.plot(convolved_signal)
        ax.set(title="Segnale + gradino", xlabel="Timestep n")
        plt.show()

    def custom_method(self):
        temp_signal = np.random.normal(size=self.__signal.shape, loc=1.0, scale=3.0)
        new_signal = self.__signal * temp_signal
        print(np.mean(new_signal < 2.0))


c_signal = CustomSignal(A=2.0, L=4, frequency=6.0, duration=3.0, sampling_rate=2000)
c_signal.plot_signal_and_fft()
convolved_signal = c_signal.convolve_and_plot(f_cutoff=8, M=80)
c_signal.add_step_signal(convolved_signal)
c_signal.custom_method()
