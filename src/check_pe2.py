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
print(f"NumberOfSections: {num_sects}")
print(f"SizeOfOptionalHeader: 0x{opt_hdr_sz:X}")

opt = coff + 20
magic = struct.unpack_from('<H', data, opt)[0]
print(f"PE32+ magic: 0x{magic:04X}")

# PE32+ fixed fields (all offsets from opt)
entry_rva = struct.unpack_from('<I', data, opt+0x10)[0]
base_of_code = struct.unpack_from('<I', data, opt+0x14)[0]
image_base = struct.unpack_from('<Q', data, opt+0x18)[0]
sect_align = struct.unpack_from('<I', data, opt+0x20)[0]
file_align = struct.unpack_from('<I', data, opt+0x24)[0]
size_of_image = struct.unpack_from('<I', data, opt+0x38)[0]
size_of_headers = struct.unpack_from('<I', data, opt+0x3C)[0]
subsystem = struct.unpack_from('<H', data, opt+0x44)[0]
num_data_dir = struct.unpack_from('<I', data, opt+0x68)[0]
data_dir_start = opt + 0x6C

print(f"\nEntryPoint RVA: 0x{entry_rva:08X}")
print(f"BaseOfCode: 0x{base_of_code:08X}")
print(f"ImageBase: 0x{image_base:016X}")
print(f"SectionAlign: 0x{sect_align:X} FileAlign: 0x{file_align:X}")
print(f"SizeOfHeaders: 0x{size_of_headers:X} SizeOfImage: 0x{size_of_image:X}")
print(f"Subsystem: {subsystem} (DLL=2)")
print(f"Data directories: {num_data_dir}")

# Data directories
dir_names = ["Export","Import","Resource","Exception","Security","BaseReloc",
             "Debug","Arch","GlobalPtr","TLS","LoadConfig","BoundImport","IAT",
             "DelayImport","CLR","Reserved"]
for i in range(min(num_data_dir, len(dir_names))):
    rva, sz = struct.unpack_from('<II', data, data_dir_start + i*8)
    if rva or sz:
        print(f"  {dir_names[i]}: RVA=0x{rva:08X} Size=0x{sz:X}")

# Section headers
sect_start = data_dir_start + num_data_dir * 8
print(f"\nSection headers at file offset 0x{sect_start:X}")
for i in range(num_sects):
    off = sect_start + i*40
    name = data[off:off+8].rstrip(b'\x00').decode('ascii', errors='replace')
    vsize = struct.unpack_from('<I', data, off+8)[0]
    vrva = struct.unpack_from('<I', data, off+12)[0]
    raw_sz = struct.unpack_from('<I', data, off+16)[0]
    raw_ptr = struct.unpack_from('<I', data, off+20)[0]
    chars = struct.unpack_from('<I', data, off+36)[0]
    print(f"  [{i}] '{name}': VSize=0x{vsize:X} VRVA=0x{vrva:X} RawSz=0x{raw_sz:X} RawPtr=0x{raw_ptr:X} Chars=0x{chars:08X}")

# Check alignment
print(f"\nCode offset in file: {size_of_headers} (0x{size_of_headers:X})")
print(f"Code RVA: {section_rva if 'section_rva' in dir() else 0x1000}")

# First 16 bytes at code offset (entry point in file)
for sect_idx in range(num_sects):
    off = sect_start + sect_idx*40
    vrva = struct.unpack_from('<I', data, off+12)[0]
    raw_ptr = struct.unpack_from('<I', data, off+20)[0]
    vsize = struct.unpack_from('<I', data, off+8)[0]
    if vrva <= entry_rva < vrva + (vsize or 0xFFFFFFFF):
        file_off = entry_rva - vrva + raw_ptr
        print(f"\nEntry point at file offset 0x{file_off:X} (RVA 0x{entry_rva:X})")
        dump = data[file_off:file_off+32]
        print(' '.join(f'{b:02X}' for b in dump))
        break
else:
    print(f"\nWARNING: Entry point RVA 0x{entry_rva:X} not in any section!")
    print(f"Sections ranges: {[(struct.unpack_from('<I', data, sect_start+i*40+12)[0], struct.unpack_from('<I', data, sect_start+i*40+8)[0]) for i in range(num_sects)]}")

# Check PeHeaderValid check
print(f"\n=== Sanity checks ===")
print(f"PointerToRawData + SizeOfRawData must be <= file size: {raw_ptr+raw_sz} <= {len(data)}") if num_sects > 0 else None
if 0x200 <= size_of_headers <= 0x200:
    print(f"SizeOfHeaders=0x{size_of_headers:X} OK (512)")
else:
    print(f"WARNING: SizeOfHeaders=0x{size_of_headers:X} unusual")
