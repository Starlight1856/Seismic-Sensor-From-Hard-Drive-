#
# python script for seismic analysis
#
import datetime
import time
import matplotlib.pyplot as plt
import numpy as np
import scipy.signal as signal
import scipy.fftpack
import sys


def low_pass_filter(data, cutoff, fs, n_taps=255):
    """Apply low-pass filter

    Parameters
    ----------
    data : array, shape (`T`, `dim`)
        Array of sequence.
    cutoff : int,
        Cutoff frequency
    fs : int,
        Sampling frequency
    n_taps : int, optional
        Tap number

    Returns
    -------
    modified data: array, shape (`T`, `dim`)
        Array of modified sequence.
    """
    if data.shape[0] < n_taps * 3:
        raise ValueError(
            'Length of data should be three times longer than n_taps.')

    fil = signal.firwin(n_taps, cutoff, pass_zero=True, nyq=fs//2)
    filt_data = signal.filtfilt(fil, 1, data, axis=0)
    return filt_data 



data = np.loadtxt("/tmp/data1_31_08_21_long.txt", dtype=int )
datasamp = np.loadtxt("/tmp/data2_31_08_21_long.txt", dtype=int )


#data = np.loadtxt("/tmp/data1.txt", dtype=int)
#datasamp = np.loadtxt("/tmp/data2.txt", dtype=int)

data_sum = sum(data)
samps = len(data)
ave = data_sum/samps

#remove offset
data = data - ave

Ts = datasamp[2]/1e6
Fs = 1/Ts

elasped_time = datasamp[1] - datasamp[0]
Ts = datasamp[2]/1e6
print("Elasped time sec=", elasped_time)
print("Ts=", Ts)
print("Sample Rate = ", 1.0/Ts, " Hz")

datetime_time_s = datetime.datetime.fromtimestamp(datasamp[0])
print("Start Time=", datetime_time_s)
datetime_time_e = datetime.datetime.fromtimestamp(datasamp[1])
print("EndTime=", datetime_time_e)


t= np.linspace(0, samps*Ts, samps)
yfilt = low_pass_filter( data, 5, 200, 1023)
print( samps )


datax=yfilt
N= samps

T = Ts
x = np.linspace(0.0, N*T, N)
yf = scipy.fftpack.fft(datax)
xf = np.linspace(0.0, 1.0//(2.0*T), N//2)

fig, ax = plt.subplots()
plt.title("FFT")
textstr = " Samp Freq=" + str(round(Fs,1) ) + "Hz"


plt.figtext(0.02, .95, "Start= ", fontsize=10)
plt.figtext(0.09, .95, datetime_time_s, fontsize=10)
plt.figtext(0.38, .95, "End= ", fontsize=10)
plt.figtext(0.44, .95, datetime_time_e, fontsize=10)
plt.figtext(0.70, .95, textstr, fontsize=10)


ax.plot(xf, 2.0/N * np.abs(yf[:N//2]))
plt.show()


sys.exit()

    

