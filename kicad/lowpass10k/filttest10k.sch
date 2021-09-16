EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "4-pole butterworth low pass filter"
Date "22 03 2021"
Rev "1.1"
Comp ""
Comment1 "Anti-aliasing filter Cut off 200Hz"
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Amplifier_Operational:OP07 U1
U 1 1 605631D4
P 4055 3355
F 0 "U1" H 4399 3401 50  0000 L CNN
F 1 "OP07" H 4399 3310 50  0000 L CNN
F 2 "" H 4105 3405 50  0001 C CNN
F 3 "https://www.analog.com/media/en/technical-documentation/data-sheets/OP07.pdf" H 4105 3505 50  0001 C CNN
F 4 "X" H 4055 3355 50  0001 C CNN "Spice_Primitive"
F 5 "OP07" H 4055 3355 50  0001 C CNN "Spice_Model"
F 6 "Y" H 4055 3355 50  0001 C CNN "Spice_Netlist_Enabled"
F 7 "/home/paul/manuals/spice/Op07.mod" H 4055 3355 50  0001 C CNN "Spice_Lib_File"
F 8 "3 2 7 4 6" H 4055 3355 50  0001 C CNN "Spice_Node_Sequence"
	1    4055 3355
	1    0    0    -1  
$EndComp
Wire Wire Line
	3755 3455 3620 3455
Wire Wire Line
	3620 3455 3620 3885
Wire Wire Line
	3620 3885 4355 3885
Wire Wire Line
	4355 3885 4355 3355
Wire Wire Line
	4355 3355 4665 3355
Connection ~ 4355 3355
$Comp
L pspice:R R2
U 1 1 60565A06
P 3010 3255
F 0 "R2" V 2805 3255 50  0000 C CNN
F 1 "5600" V 2896 3255 50  0000 C CNN
F 2 "" H 3010 3255 50  0001 C CNN
F 3 "~" H 3010 3255 50  0001 C CNN
	1    3010 3255
	0    1    1    0   
$EndComp
$Comp
L pspice:R R1
U 1 1 60566468
P 2350 3255
F 0 "R1" V 2145 3255 50  0000 C CNN
F 1 "5600" V 2236 3255 50  0000 C CNN
F 2 "" H 2350 3255 50  0001 C CNN
F 3 "~" H 2350 3255 50  0001 C CNN
	1    2350 3255
	0    1    1    0   
$EndComp
Wire Wire Line
	3755 3255 3370 3255
Wire Wire Line
	2760 3255 2675 3255
$Comp
L pspice:VSOURCE Vin1
U 1 1 6056A3F0
P 2555 4860
F 0 "Vin1" H 2783 4906 50  0000 L CNN
F 1 "dc 0 ac 1" H 2783 4815 50  0000 L CNN
F 2 "" H 2555 4860 50  0001 C CNN
F 3 "~" H 2555 4860 50  0001 C CNN
	1    2555 4860
	1    0    0    -1  
$EndComp
$Comp
L pspice:VSOURCE V+1
U 1 1 6056AFF3
P 3715 4870
F 0 "V+1" H 3943 4916 50  0000 L CNN
F 1 "5" H 3943 4825 50  0000 L CNN
F 2 "" H 3715 4870 50  0001 C CNN
F 3 "~" H 3715 4870 50  0001 C CNN
	1    3715 4870
	1    0    0    -1  
$EndComp
$Comp
L pspice:VSOURCE V-1
U 1 1 6056B3ED
P 4765 4865
F 0 "V-1" H 4993 4911 50  0000 L CNN
F 1 "-5" H 4993 4820 50  0000 L CNN
F 2 "" H 4765 4865 50  0001 C CNN
F 3 "~" H 4765 4865 50  0001 C CNN
	1    4765 4865
	1    0    0    -1  
$EndComp
Wire Wire Line
	3370 3255 3370 3595
Connection ~ 3370 3255
Wire Wire Line
	3370 3255 3260 3255
$Comp
L pspice:C C1
U 1 1 6057D250
P 3370 3845
F 0 "C1" H 3548 3891 50  0000 L CNN
F 1 "130nF" H 3548 3800 50  0000 L CNN
F 2 "" H 3370 3845 50  0001 C CNN
F 3 "~" H 3370 3845 50  0001 C CNN
	1    3370 3845
	1    0    0    -1  
$EndComp
Connection ~ 2675 3255
Wire Wire Line
	2675 3255 2600 3255
Wire Wire Line
	2675 2530 2675 3255
Wire Wire Line
	2675 2530 3250 2530
$Comp
L pspice:C C2
U 1 1 6057FF17
P 3500 2530
F 0 "C2" V 3185 2530 50  0000 C CNN
F 1 "150nF" V 3276 2530 50  0000 C CNN
F 2 "" H 3500 2530 50  0001 C CNN
F 3 "~" H 3500 2530 50  0001 C CNN
	1    3500 2530
	0    1    1    0   
$EndComp
Wire Wire Line
	3750 2530 4665 2530
Wire Wire Line
	4665 2530 4665 3355
Connection ~ 4665 3355
Wire Wire Line
	4665 3355 4810 3355
$Comp
L pspice:0 #GND01
U 1 1 60581C3A
P 2555 5160
F 0 "#GND01" H 2555 5060 50  0001 C CNN
F 1 "0" H 2555 5249 50  0000 C CNN
F 2 "" H 2555 5160 50  0001 C CNN
F 3 "~" H 2555 5160 50  0001 C CNN
	1    2555 5160
	1    0    0    -1  
$EndComp
$Comp
L pspice:0 #GND03
U 1 1 60582AE9
P 3715 5170
F 0 "#GND03" H 3715 5070 50  0001 C CNN
F 1 "0" H 3715 5259 50  0000 C CNN
F 2 "" H 3715 5170 50  0001 C CNN
F 3 "~" H 3715 5170 50  0001 C CNN
	1    3715 5170
	1    0    0    -1  
$EndComp
$Comp
L pspice:0 #GND04
U 1 1 60582F35
P 4765 5165
F 0 "#GND04" H 4765 5065 50  0001 C CNN
F 1 "0" H 4765 5254 50  0000 C CNN
F 2 "" H 4765 5165 50  0001 C CNN
F 3 "~" H 4765 5165 50  0001 C CNN
	1    4765 5165
	1    0    0    -1  
$EndComp
$Comp
L pspice:0 #GND02
U 1 1 60583CA3
P 3370 4095
F 0 "#GND02" H 3370 3995 50  0001 C CNN
F 1 "0" H 3370 4184 50  0000 C CNN
F 2 "" H 3370 4095 50  0001 C CNN
F 3 "~" H 3370 4095 50  0001 C CNN
	1    3370 4095
	1    0    0    -1  
$EndComp
Text GLabel 2555 4560 1    50   Input ~ 0
IN
Text GLabel 2100 3255 0    50   Input ~ 0
IN
Text GLabel 3715 4570 1    50   Input ~ 0
VP
Text GLabel 3955 3055 1    50   Input ~ 0
VP
Text GLabel 4765 4565 1    50   Input ~ 0
VM
Text GLabel 3955 3655 3    50   Input ~ 0
VM
$Comp
L Amplifier_Operational:OP07 U2
U 1 1 6058905A
P 6765 3455
F 0 "U2" H 7109 3501 50  0000 L CNN
F 1 "OP07" H 7109 3410 50  0000 L CNN
F 2 "" H 6815 3505 50  0001 C CNN
F 3 "https://www.analog.com/media/en/technical-documentation/data-sheets/OP07.pdf" H 6815 3605 50  0001 C CNN
F 4 "X" H 6765 3455 50  0001 C CNN "Spice_Primitive"
F 5 "OP07" H 6765 3455 50  0001 C CNN "Spice_Model"
F 6 "Y" H 6765 3455 50  0001 C CNN "Spice_Netlist_Enabled"
F 7 "/home/paul/manuals/spice/Op07.mod" H 6765 3455 50  0001 C CNN "Spice_Lib_File"
F 8 "3 2 7 4 6" H 6765 3455 50  0001 C CNN "Spice_Node_Sequence"
	1    6765 3455
	1    0    0    -1  
$EndComp
Wire Wire Line
	6465 3555 6330 3555
Wire Wire Line
	6330 3555 6330 3985
Wire Wire Line
	6330 3985 7065 3985
Wire Wire Line
	7065 3985 7065 3455
Wire Wire Line
	7065 3455 7375 3455
Connection ~ 7065 3455
$Comp
L pspice:R R3
U 1 1 60589138
P 5060 3355
F 0 "R3" V 4855 3355 50  0000 C CNN
F 1 "6800" V 4946 3355 50  0000 C CNN
F 2 "" H 5060 3355 50  0001 C CNN
F 3 "~" H 5060 3355 50  0001 C CNN
	1    5060 3355
	0    1    1    0   
$EndComp
Wire Wire Line
	6465 3355 6080 3355
Wire Wire Line
	5470 3355 5385 3355
Connection ~ 6080 3355
Wire Wire Line
	6080 3355 5970 3355
$Comp
L pspice:C C3
U 1 1 60589147
P 6080 3945
F 0 "C3" H 6258 3991 50  0000 L CNN
F 1 "39nF" H 6258 3900 50  0000 L CNN
F 2 "" H 6080 3945 50  0001 C CNN
F 3 "~" H 6080 3945 50  0001 C CNN
	1    6080 3945
	1    0    0    -1  
$EndComp
Connection ~ 5385 3355
Wire Wire Line
	5385 3355 5310 3355
Wire Wire Line
	5385 2630 5385 3355
Wire Wire Line
	5385 2630 5960 2630
$Comp
L pspice:C C4
U 1 1 60589155
P 6210 2630
F 0 "C4" V 5895 2630 50  0000 C CNN
F 1 "300nF" V 5986 2630 50  0000 C CNN
F 2 "" H 6210 2630 50  0001 C CNN
F 3 "~" H 6210 2630 50  0001 C CNN
	1    6210 2630
	0    1    1    0   
$EndComp
Wire Wire Line
	6460 2630 7375 2630
Wire Wire Line
	7375 2630 7375 3455
Connection ~ 7375 3455
Wire Wire Line
	7375 3455 7520 3455
$Comp
L pspice:0 #GND05
U 1 1 60589163
P 6080 4195
F 0 "#GND05" H 6080 4095 50  0001 C CNN
F 1 "0" H 6080 4284 50  0000 C CNN
F 2 "" H 6080 4195 50  0001 C CNN
F 3 "~" H 6080 4195 50  0001 C CNN
	1    6080 4195
	1    0    0    -1  
$EndComp
Text GLabel 6665 3155 1    50   Input ~ 0
VP
Text GLabel 6665 3755 3    50   Input ~ 0
VM
Wire Wire Line
	6080 3355 6080 3695
Text GLabel 7520 3455 2    50   Input ~ 0
OUT
Text Notes 2320 5910 0    50   ~ 0
.ac dec 10 .1 10k
$Comp
L pspice:R R4
U 1 1 6058912E
P 5720 3355
F 0 "R4" V 5515 3355 50  0000 C CNN
F 1 "6800" V 5606 3355 50  0000 C CNN
F 2 "" H 5720 3355 50  0001 C CNN
F 3 "~" H 5720 3355 50  0001 C CNN
	1    5720 3355
	0    1    1    0   
$EndComp
$EndSCHEMATC
