
Seismic Sensor from Old Hard Disks?
___________________________________

Whilst in the process of dismantling some old hard drives for
useful components (motors etc) it occured to me that the
voice coil/actuator magnet combination could be used to detect
motion. I also wondered if this be sensitive enough to act as
a seismic sensor. I began an project to see if it was possible.

I set out some basic requirements;

1. Use as many components that were at hand.
2. Operate the sensor as a remote autonomous system.
3. Acquire at least a days work of data which can be uploaded via WiFi.


This repository contains the work behind this experiment.

The seismic4.pdf document in the /doc folder provides more detailed
information on the work and results.



Repository Contents 
___________________________________


Beaglebone code
---------------
Code for driving PRU, and userpace application seismic3 which 
acquires and stores data, and also acts as remote server.

client3
-------
Client application for desktop which obtains and stores to file
the data stored in Beaglebone remote server.

datasheets
----------
This folder contains all the datasheets used for the project.

doc
---
seismic4.pdf document details the work and results.

kicad
-----
Project files for Kicad application. Used for circuit schematics and
lowpass filter simulation.

lpfilter
--------
Simulation results, and information used in design.

misc
----
Miscellaneous diagrams, pictures.

python
------
Python scripts used in analysis of data.

results
-------
Data files of run detailed in seismic4.pdf document,
