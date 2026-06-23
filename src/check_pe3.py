import struct, sys

path = sys.argv[1]
with open(path, 'rb') as f:
    data = f.read()

e_lfanew = struct.unpack_from('<I', data, 0x3C)[0]
sig = struct.unpack_from('<I', data, e_lfanew)[0]
assert sig == 0x4550, "Not PE"

coff = e_lfanew + 4
num_sects = struct.unpack_from('<H', data, coff+2)[0]
opt_hdr_sz = struct.unpack_from('<H', data, coff+16)[0]
print(f"Sections: {num_sects}, OptHdrSz: 0x{opt_hdr_sz:X}")

opt = coff + 20
magic = struct.unpack_from('<H', data, opt)[0]
print(f"PE32+ magic: 0x{magic:04X}")

# PE32+ correct offsets
entry_rva = struct.unpack_from('<I', data, opt+0x10)[0]
base_of_code = struct.unpack_from('<I', data, opt+0x14)[0]
image_base = struct.unpack_from('<Q', data, opt+0x18)[0]
sect_align = struct.unpack_from('<I', data, opt+0x20)[0]
file_align = struct.unpack_from('<I', data, opt+0x24)[0]
size_of_image = struct.unpack_from('<I', data, opt+0x38)[0]
size_of_headers = struct.unpack_from('<I', data, opt+0x3C)[0]
subsystem = struct.unpack_from('<H', data, opt+0x44)[0]
num_data_dir = struct.unpack_from('<I', data, opt+0x6C)[0]
dir_start = opt + 0x70

print(f"Entry: 0x{entry_rva:X}, ImageBase: 0x{image_base:X}")
print(f"SectAlign: 0x{sect_align:X}, FileAlign: 0x{file_align:X}")
print(f"SizeOfHeaders: 0x{size_of_headers:X}, SizeOfImage: 0x{size_of_image:X}")
print(f"Subsystem: {subsystem} (DLL=2), DataDirs: {num_data_dir}")

dir_names = ["Export","Import","Resource","Exception","Security","BaseReloc",
             "Debug","Arch","GlobalPtr","TLS","LoadConfig","BoundImport","IAT",
             "DelayImport","CLR","Reserved"]
for i in range(min(num_data_dir, 16)):
    rva, sz = struct.unpack_from('<II', data, dir_start + i*8)
    if rva or sz:
        print(f"  {dir_names[i]}: RVA=0x{rva:X} Size=0x{sz:X}")

# Section headers
sect_start = dir_start + num_data_dir * 8
print(f"\nSection header at file offset 0x{sect_start:X}")
for i in range(num_sects):
    off = sect_start + i*40
    name = data[off:off+8].rstrip(b'\x00').decode('ascii', errors='replace')
    vsize = struct.unpack_from('<I', data, off+8)[0]
    vrva = struct.unpack_from('<I', data, off+12)[0]
    raw_sz = struct.unpack_from('<I', data, off+16)[0]
    raw_ptr = struct.unpack_from('<I', data, off+20)[0]
    chars = struct.unpack_from('<I', data, off+36)[0]
    print(f"  '{name}': VSize=0x{vsize:X} VRVA=0x{vrva:X} RawSz=0x{raw_sz:X} RawPtr=0x{raw_ptr:X} Chars=0x{chars:08X}")
    if chars == 0x60000020: print("    -> RX (code)")
    elif chars == 0xE0000020: print("    -> RWX (code)")

# Check code size
total_code_and_exports = raw_sz if num_sects > 0 else 0
print(f"\nTotal raw size: 0x{total_code_and_exports:X} bytes")
expected_file_size = size_of_headers + total_code_and_exports
print(f"Expected file size: 0x{expected_file_size:X} = {expected_file_size}")
print(f"Actual file size: {len(data)}")
if len(data) != expected_file_size:
    print(f"MISMATCH: {len(data) - expected_file_size} extra bytes")
else:
    print("File size matches expected")

# Entry point
for i in range(num_sects):
    off = sect_start + i*40
    vrva = struct.unpack_from('<I', data, off+12)[0]
    raw_ptr = struct.unpack_from('<I', data, off+20)[0]
    vsize = struct.unpack_from('<I', data, off+8)[0]
    if vrva <= entry_rva < vrva + (vsize if vsize else 0x7FFFFFFF):
        foff = entry_rva - vrva + raw_ptr
        print(f"\nEntry at file offset 0x{foff:X}:")
        dump = data[foff:foff+24]
        print(' '.join(f'{b:02X}' for b in dump))
        break
else:
    print(f"\nEntry RVA 0x{entry_rva:X} NOT IN SECTION [0x{vrva:X}, 0x{vrva+vsize:X})")
