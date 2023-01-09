#!/bin/python

# ##############################################################################
# Description  : Utility script for VHDL design LUT
# File         : LUT_generation.py
# Author(s)    : Luca Fanucci luca.fanucci@unipi.it
#              : Massimiliano Donati massimiliano.donati@unipi.it
#              : Luca Zulberti <luca.zulberti@phd.unipi.it>
# Department   : Dept. Information Engineering, University of Pisa
# Created      : Wed May 9 14:58:06 2018
# Last update  : 20/04/2022
# Language     : python3.x
# ##############################################################################

import math

print("")
print("*" * 80)
print("** DDFS VHDL automatic LUT generation script")
print("** luca.fanucci@unipi.it")
print("** massimiliano.donati@unipi.it")
print("** luca.zulberti@phd.unipi.it")
print("*" * 80)
print("")

N_FW = int(input('Frequency Word width: '))
while N_FW < 1:
	print("WARNING: Insert positive integer")
	N_FW = int(input('Frequency Word width: '))

N_YQ = int(input('Output Quantized width: '))
while N_YQ < 1:
	print("WARNING: Insert positive integer")
	int(input('Output Quantized width: '))

# SG_UN = raw_input('LUT type SG|UN: ')
# while SG_UN != "SG" and SG_UN != "UN":
	# print("WARNING: BAD choice")
	# SG_UN = raw_input('LUT type SG|UN: ')

# Up to now, SG_UN is forced to SG (signed LUT)
SG_UN = "SG"

N_LUT_line = 2**N_FW
lsb = 1/(2**(float(N_YQ)-1)-1) # anyway balanced

print("-" * 40)
print("N_FW  : " + str(N_FW))
print("lines : " + str(N_LUT_line))
print("N_YQ  : " + str(N_YQ))
print("SG_UN : " + SG_UN)
print("lsb   : " + str(lsb))

mname = "ddfs_lut_" + str(N_LUT_line) + "_" + str(N_YQ) + "bit"
fname = mname + ".vhd"

# start to write the file
out_file = open(fname, "w")

out_file.write("library IEEE;\n")
out_file.write("  use IEEE.std_logic_1164.all;\n")
out_file.write("  use IEEE.numeric_std.all;\n")
out_file.write("\n")
out_file.write("entity " + mname + " is\n")
out_file.write("  port (\n")
out_file.write("    address  : in  std_logic_vector(" + str(N_FW-1) + " downto 0);\n")
out_file.write("    ddfs_out : out std_logic_vector(" + str(N_YQ-1) + " downto 0)\n")
out_file.write("  );\n")
out_file.write("end entity;\n")
out_file.write("\n")
out_file.write("architecture rtl of " + mname + " is\n")
out_file.write("\n")
out_file.write("  type LUT_t is array (natural range 0 to " + str(N_LUT_line-1) + ") of integer;\n")
out_file.write("  constant LUT: LUT_t := (\n")

for a in range(N_LUT_line):
	x = round((math.sin(2*math.pi*a/N_LUT_line))/lsb)

	if SG_UN == "UN":
		x += 2**(N_YQ-1) - 1

	if a<(N_LUT_line-1):
		out_file.write("    " + str(a) + " => " + str(int(x)) +",\n")
	else:
		out_file.write("    " + str(a) + " => " + str(int(x)) +"\n")

out_file.write("  );\n")
out_file.write("\n")
out_file.write("begin\n")

if SG_UN == "UN":
	out_file.write("  ddfs_out <= std_logic_vector(to_unsigned(LUT(to_integer(unsigned(address)))," + str(N_YQ) + "));\n")
else:
	out_file.write("  ddfs_out <= std_logic_vector(to_signed(LUT(to_integer(unsigned(address)))," + str(N_YQ) + "));\n")

out_file.write("end architecture;\n")

out_file.close()
