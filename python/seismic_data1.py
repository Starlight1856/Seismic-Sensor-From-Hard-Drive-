#
# python script for seismic data 
#
import datetime
import time
import matplotlib.pyplot as plt
import numpy as np
import scipy.signal as signal
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

# remove offset
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

num_graphs = round(( (samps*Ts)/1800) + 0.5 )
print("Length=", samps )
print("Time=", datetime_time_e - datetime_time_s)
print("Num Graphs=", num_graphs )

# select low pass filter to be applied
yfilt = low_pass_filter( data, 20, 200, 1023)
print( samps )

graph_seconds = 1800
graph_plot_size = round(graph_seconds * Fs)


textstr = " Samp Freq=" + str(round(Fs,1) ) + "Hz"

plt.figtext(0.02, .95, "Start= ", fontsize=10)
plt.figtext(0.09, .95, datetime_time_s, fontsize=10)
plt.figtext(0.38, .95, "End= ", fontsize=10)
plt.figtext(0.44, .95, datetime_time_e, fontsize=10)
plt.figtext(0.70, .95, textstr, fontsize=10)



for i in range(num_graphs):
  plt.subplot( round((num_graphs/2)+0.5), 2, i+1)
  startpos= i*graph_plot_size
  endpos= (i+1) * graph_plot_size
  plt.plot(t[startpos:endpos], yfilt[startpos:endpos],  color = 'g')
  plt.xlim( graph_seconds * i, graph_seconds* (i+1) )
  plt.ylim(-500, 500)
  #plt.grid(True)
 




plt.show()


sys.exit()

    

