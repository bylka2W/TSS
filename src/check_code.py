import struct, sys
f=open(sys.argv[1],'rb')
d=f.read()
f.close()
pe_off = struct.unpack_from('<I', d, 0x3C)[0]
coff_hdr_off = pe_off + 4
num_sections = struct.unpack_from('<H', d, coff_hdr_off+2)[0]
size_opt_hdr = struct.unpack_from('<H', d, coff_hdr_off+16)[0]
opt_hdr_off = pe_off + 24
magic = struct.unpack_from('<H', d, opt_hdr_off)[0]
shdr_off = opt_hdr_off + size_opt_hdr
# For each export, find its code
exp_dir_rva_off = opt_hdr_off + (96 if magic==0x10B else 112)
exp_dir_rva = struct.unpack_from('<I', d, exp_dir_rva_off)[0]
exp_dir_size = struct.unpack_from('<I', d, exp_dir_rva_off+4)[0]
print(f'Export dir: RVA={hex(exp_dir_rva)}, size={exp_dir_size}')
for i in range(num_sections):
    shdr = shdr_off + i*40
    name = d[shdr:shdr+8].split(b'\x00')[0]
    vaddr = struct.unpack_from('<I', d, shdr+12)[0]
    vsize = struct.unpack_from('<I', d, shdr+8)[0]
    raw = struct.unpack_from('<I', d, shdr+20)[0]
    rawsz = struct.unpack_from('<I', d, shdr+16)[0]
    print(f'Section {name!r}: vaddr={hex(vaddr)} vsize={vsize} raw={hex(raw)}/{rawsz}')
    if vaddr <= exp_dir_rva < vaddr+vsize and exp_dir_size>0:
        off = raw + (exp_dir_rva - vaddr)
        num_names = struct.unpack_from('<I', d, off+24)[0]
        addr_fns = struct.unpack_from('<I', d, off+28)[0]
        addr_nms = struct.unpack_from('<I', d, off+32)[0]
        addr_ord = struct.unpack_from('<I', d, off+36)[0]
        print(f'Num exports: {num_names}')
        for j in range(num_names):
            nrva = struct.unpack_from('<I', d, raw + (addr_nms - vaddr) + j*4)[0]
            noff = raw + (nrva - vaddr)
            ename = d[noff:].split(b'\x00')[0].decode()
            orv = struct.unpack_from('<H', d, raw + (addr_ord - vaddr) + j*2)[0]
            frv = struct.unpack_from('<I', d, raw + (addr_fns - vaddr) + orv*4)[0]
            foff = raw + (frv - vaddr)
            print(f'\n{ename} at RVA={hex(frv)}, file offset={hex(foff)}')
            # Dump 64 bytes of code
            code = d[foff:foff+64]
            print(' '.join(f'{b:02x}' for b in code))
