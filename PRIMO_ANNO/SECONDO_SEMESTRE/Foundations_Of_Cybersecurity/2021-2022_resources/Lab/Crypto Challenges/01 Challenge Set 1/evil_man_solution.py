#!/usr/bin/env python3
import random
import time
import sys
#load the file in enc
f= open("deepest_secrets.txt", "rb")
enc=list(f.read())
#I extract the seed from the last bytes of the file.
#I don't know the length of the seed, so i use a sample return from the function time()
sample=str(time.time()).encode('ASCII')
enc_time = enc[len(enc)-len(sample)+1:len(enc)]
dec_time = []
for i in enc_time:
	dec_time.append(i^0x88)
#rebuilding the seed as a string
enc_seed = ''.join([chr(i) for i in dec_time])
#converting the seed to float
seed = float(enc_seed)
print('The random seed used for key generation is: '+ enc_seed)
#Seed the generator. No matter how good it is, if i have the seed i have the key!
random.seed(seed)
#Generate the key, remember last bytes are not encrypted with the key!
key = [random.randrange(256) for _ in enc[:len(enc)-len(sample)+1]]
print('The key used for encryption is' + str(key))
#decrypt and print the flag.
dec_msg = [c^k for (c,k) in zip (enc[:len(enc)-len(sample)+1],key)]
print(''.join([chr(i) for i in dec_msg]))
