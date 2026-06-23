import struct
f = open('C:/TSS/test_ret_float.dll','rb')
d = f.read()
f.close()
off = 0x4fd
code = d[off:off+200]
for k in range(len(code)-4):
    b3 = code[k:k+3]
    b4 = code[k:k+4]
    if b3 == bytes([0xf3, 0x0f, 0x58]):
        print(f'ADDSS at {hex(off+k)}')
    if b3 == bytes([0xf3, 0x0f, 0x11]):
        print(f'MOVSS store at {hex(off+k)}')
    if b4 == bytes([0x66, 0x0f, 0x7e, 0xc0]):
        print(f'MOVD EAX, XMM0 at {hex(off+k)}')
    if b3 == bytes([0xf3, 0x0f, 0x10]):
        print(f'MOVSS load at {hex(off+k)}')
