#!/usr/bin/env python3
import random
import sys
import time

# if you think about the plaintext of the file:
# position(0-f)	|0123456789abcdef|

# block#1		|Alice transfers |<-- the last character (position f) is a whitespace
# block#2		|0000000000100000|
# block#3		|$ to: PisaUniver|<-- (1),(5) are whitespaces
# block#4		|sity. Transfer# |<-- (5),(f) are whitespaces
# block#5		|0000001352782121|<-- you don't know the exact number, but you know is on 16 bytes!
# block#6		|paddingpaddingpa|

#the solution is to swap block#2 with block#5

#Read the ciphertext
f= open("transaction.enc", "rb")
ct=list(f.read())

#Slice each block
first_block	=  	[i for i in ct[0:16]]
second_block= 	[i for i in ct[16:32]]
third_block	=	[i for i in ct[32:48]]
fourth_block=	[i for i in ct[48:64]]
fifth_block	=	[i for i in ct[64:80]]
padding 	= 	[i for i in ct[80:]]

#Build the ct swapping 2nd and 5th blocks
forged_ct = first_block + fifth_block + third_block + fourth_block + second_block + padding
#Save it in forged.txt
with open("forged.txt", "wb") as f:
    f.write(bytes(forged_ct))
