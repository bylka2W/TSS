import struct, sys

path = sys.argv[1]
with open(path, 'rb') as f:
    data = f.read()

# DOS header
e_magic = struct.unpack_from('<H', data, 0)[0]
e_lfanew = struct.unpack_from('<I', data, 0x3C)[0]
print(f"e_magic: 0x{e_magic:04X}")
print(f"e_lfanew: 0x{e_lfanew:08X} ({e_lfanew})")

# PE signature
sig = struct.unpack_from('<I', data, e_lfanew)[0]
print(f"PE sig: 0x{sig:08X}")
if sig != 0x4550:
    print("NOT a PE file!")
    sys.exit(1)

# COFF header
coff = e_lfanew + 4
machine = struct.unpack_from('<H', data, coff)[0]
num_sects = struct.unpack_from('<H', data, coff+2)[0]
print(f"Machine: 0x{machine:04X} (x64=0x8664)")
print(f"Number of sections: {num_sects}")

# Optional header
opt = coff + 20
magic = struct.unpack_from('<H', data, opt)[0]
print(f"Optional header magic: 0x{magic:04X} (PE32+=0x20B)")

# PE32+ fields
entry_rva = struct.unpack_from('<Q', data, opt+16)[0] if magic == 0x20B else struct.unpack_from('<I', data, opt+16)[0]
image_base = struct.unpack_from('<Q', data, opt+24)[0] if magic == 0x20B else struct.unpack_from('<I', data, opt+24)[0]
sect_align = struct.unpack_from('<I', data, opt+32)[0] if magic == 0x20B else struct.unpack_from('<I', data, opt+36)[0]
file_align = struct.unpack_from('<I', data, opt+36)[0] if magic == 0x20B else struct.unpack_from('<I', data, opt+40)[0]
size_of_headers = struct.unpack_from('<I', data, opt+60)[0] if magic == 0x20B else struct.unpack_from('<I', data, opt+64)[0]
# For PE32+, check offsets
if magic == 0x20B:
    # PE32+ layout: after magic(2)+major(1)+minor(1)+sizeOfCode(4)+sizeOfInitData(4)+sizeOfUninitData(4)+entryPoint(4)+baseOfCode(4)+imageBase(8)=32
    # Actually let me recalculate:
    # offset 0: magic (2)
    # offset 2: major linker ver (1)
    # offset 3: minor linker ver (1)
    # offset 4: size of code (4)
    # offset 8: size of init data (4)
    # offset 12: size of uninit data (4)
    # offset 16: entry point RVA (4)
    # offset 20: base of code (4)
    # offset 24: image base (8)
    # offset 32: section alignment (4)
    # offset 36: file alignment (4)
    # offset 40: major OS ver (2)
    # offset 42: minor OS ver (2)
    # offset 44: major image ver (2)
    # offset 46: minor image ver (2)
    # offset 48: major subsystem ver (2)
    # offset 50: minor subsystem ver (2)
    # offset 52: Win32 version value (4)
    # offset 56: size of image (4)
    # offset 60: size of headers (4)

    # Let me just recalculate from opt directly
    entry_rva = struct.unpack_from('<I', data, opt+16)[0]
    base_of_code = struct.unpack_from('<I', data, opt+20)[0]
    image_base = struct.unpack_from('<Q', data, opt+24)[0]
    sect_align = struct.unpack_from('<I', data, opt+32)[0]
    file_align = struct.unpack_from('<I', data, opt+36)[0]
    size_of_image = struct.unpack_from('<I', data, opt+56)[0]
    size_of_headers = struct.unpack_from('<I', data, opt+60)[0]
    
    subsystem = struct.unpack_from('<H', data, opt+68)[0]
    num_data_dir = struct.unpack_from('<I', data, opt+92)[0]
    data_dir_start = opt + 96

print(f"EntryPoint RVA: 0x{entry_rva:08X}")
print(f"BaseOfCode: 0x{base_of_code:08X}")
print(f"ImageBase: 0x{image_base:016X}")
print(f"SectionAlignment: 0x{sect_align:08X}")
print(f"FileAlignment: 0x{file_align:08X}")
print(f"SizeOfHeaders: 0x{size_of_headers:08X}")
print(f"SizeOfImage: 0x{size_of_image:08X}")
print(f"Subsystem: {subsystem} (DLL=2)")

# Data directories
print(f"\nData directories ({num_data_dir}):")
dir_names = ["Export", "Import", "Resource", "Exception", "Security", "BaseReloc", "Debug", "Arch", "GlobalPtr", "TLS", "LoadConfig", "BoundImport", "IAT", "DelayImport", "CLR", "Reserved"]
for i in range(min(num_data_dir, len(dir_names))):
    rva, size = struct.unpack_from('<II', data, data_dir_start + i*8)
    if rva or size:
        print(f"  {dir_names[i]}: RVA=0x{rva:08X}, Size=0x{size:08X}")

# Sections
print(f"\nSections ({num_sects}):")
sect_header_start = opt + (96 + num_data_dir * 8 if magic == 0x20B else (96 + 16 * 8 if magic == 0x10B else 0))
# Actually the section headers start right after the data directories
# For PE32+, each data dir entry is 8 bytes, so total = 96 + num_data_dir * 8
sect_header_start = opt + (96 + num_data_dir * 8)
for i in range(num_sects):
    off = sect_header_start + i * 40
    name = data[off:off+8].rstrip(b'\x00').decode('ascii', errors='replace')
    vsize = struct.unpack_from('<I', data, off+8)[0]
    vrva = struct.unpack_from('<I', data, off+12)[0]
    raw_size = struct.unpack_from('<I', data, off+16)[0]
    raw_ptr = struct.unpack_from('<I', data, off+20)[0]
    reloc_ptr = struct.unpack_from('<I', data, off+24)[0]
    linenum_ptr = struct.unpack_from('<I', data, off+28)[0]
    num_reloc = struct.unpack_from('<H', data, off+32)[0]
    num_linenum = struct.unpack_from('<H', data, off+34)[0]
    chars = struct.unpack_from('<I', data, off+36)[0]
    print(f"  {name}: VSize=0x{vsize:X}, VRVA=0x{vrva:X}, RawSize=0x{raw_size:X}, RawPtr=0x{raw_ptr:X}")
    print(f"    Characteristics: 0x{chars:08X} {'(RX)' if chars==0x60000020 else '(RWX)' if chars==0xE0000020 else ''}")

# Check code at EntryPoint
print(f"\nCode at EntryPoint (RVA 0x{entry_rva:08X}):")
for sect_idx in range(num_sects):
    off = sect_header_start + sect_idx * 40
    sect_vrva = struct.unpack_from('<I', data, off+12)[0]
    sect_raw = struct.unpack_from('<I', data, off+20)[0]
    sect_vsize = struct.unpack_from('<I', data, off+8)[0]
    if sect_vrva <= entry_rva < sect_vrva + sect_vsize:
        file_off = entry_rva - sect_vrva + sect_raw
        code_bytes = data[file_off:file_off+32]
        print(f"  Found in section {sect_idx}, file offset 0x{file_off:X}")
        print(f"  Bytes: {' '.join(f'{b:02X}' for b in code_bytes)}")
        break
