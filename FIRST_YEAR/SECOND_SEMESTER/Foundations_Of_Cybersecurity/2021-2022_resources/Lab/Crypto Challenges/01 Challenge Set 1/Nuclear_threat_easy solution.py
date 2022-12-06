#!/usr/bin/env python3
import sys
import random
A=1103515245
C=12345
n=2**31
f= open("captured_ct_easy.txt", "rb")
ct=list(f.read())
#I find k0 bytes by xoring b'From' with ct[0]
test='From: '.encode('ASCII')
ki_b=[test[0]^ct[0], test[1]^ct[1], test[2]^ct[2], test[3]^ct[3]]
#I rebuild the 32-bit integer k0
ki=ki_b[3]<<24|ki_b[2]<<16|ki_b[1]<<8|ki_b[0]
print(ki)
pt=[]

for i in range(int(len(ct)/4)):
	#this decrypts the block
	pt+=[a^b for (a,b) in zip(ki_b,ct[i*4:i*4+4])]
	#logically, is the same as doing:
	#pt.append(ki_b[0] ^ ct[i*4+0])
	#pt.append(ki_b[1] ^ ct[i*4+1])
	#pt.append(ki_b[2] ^ ct[i*4+2])
	#pt.append(ki_b[3] ^ ct[i*4+3])

	#I compute the next 32-bit integer of LCG
	ki = (A*ki + C)%n
	#I split the 32-bit integer in 8-byte blocks
	ki_b = [ki%256, (ki>>8)%256, (ki>>16)%256, (ki>>24)%256]
print(''.join([chr(i) for i in pt]))

