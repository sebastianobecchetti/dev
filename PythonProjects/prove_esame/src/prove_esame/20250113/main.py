import numpy as np
class Signal():
    def __init__(self, A,f_m, f_c,m,duration,sampling_rate):
        self.__A = A
        self.__f_m = f_m
        self.__f_c = f_c
        self.__m = m
        self.__duration = duration
        self.__sampling_rate = sampling_rate

        self.__time_index = np.linspace(0,self.__duration, self.__duration*self.__sampling_rate)
        x = np.arange(self.__duration)
        for t in self.__time_index:
            x += self.__A*(1+self.__m*np.sin(2*np.pi*self.__f_m*self.__time_index))+np.cos(2*np.pi*self.__f_c*self.__time_index)

