#!/usr/bin/env python3
import sys
import random
f= open("captured_ct.txt", "rb")
ct=list(f.read())
#I find k0 by xoring b'F' with ct[0]
test='From: '.encode('ASCII')
k0=test[0]^ct[0]
print(k0)
A=0
C=0
n=256
found = False
#I bruteforce A and C in [0,255]
#Module operation is distributive, there is no need to search beyond 255!
for a in range(256): #iterate 'a' in [0,255]
	if not(found):
		for c in range(256): #iterate 'a' in [0,255]
			if not(found):
				k=k0 #Key must me reset when trying a new combination of a,b
				for i in range(len(test)):#iterate 'a' in [0,6]
					if k^ct[i] == test[i]: #Known-Plaintext attack
						if i == len(test)-1: #If all six characters match, i reasonably found a collision!
							A=a
							C=c
							found = True
							break
						else: 
							k=(a*k+c)%n
					else: break
pt=[]
ki=k0
for i in range(len(ct)):
	pt.append(ki ^ ct[i])
	ki = (A*ki + C)%n
print(''.join([chr(i) for i in pt]))

