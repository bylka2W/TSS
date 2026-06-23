import struct

with open('C:/TSS/test_persist.dll', 'rb') as f:
    data = f.read()

print(f"File size: {len(data)}")

# .text section: raw=0x200, RVA=0x1000, size=0x1682
text_raw = 0x200
text_size = 0x1682

# Check the IAT entry for TlsAlloc (index 28)
# The IAT is at the end of the code section
# Let me find it by looking at the last FF 15 target in DllMain

# The TlsAlloc call is at file offset 0x27, with disp 0x15e1
# IAT entry file offset = 0x27 + 6 + 0x15e1 = 0x180e
iat_entry_off = 0x180e
print(f"\nIAT entry at file offset 0x{iat_entry_off:x}:")
iat_va = struct.unpack_from('<Q', data, iat_entry_off)[0]
print(f"  Value (before load): 0x{iat_va:016x}")

# This should be an RVA pointing to a hint/name entry
hint_name_rva = iat_va & 0xFFFFFFFF
if hint_name_rva > 0x1000:
    hint_name_off = (hint_name_rva - 0x1000) + text_raw
    print(f"  Hint/name RVA: 0x{hint_name_rva:x}")
    print(f"  Hint/name file offset: 0x{hint_name_off:x}")
    if hint_name_off < len(data) - 4:
        hint = struct.unpack_from('<H', data, hint_name_off)[0]
        name_start = hint_name_off + 2
        name_end = data.index(b'\x00', name_start) if b'\x00' in data[name_start:name_start+50] else name_start+50
        name = data[name_start:name_end].decode('ascii', errors='replace')
        print(f"  Hint: {hint}, Name: '{name}'")
else:
    print(f"  Not a valid hint/name RVA")

# Also check what's at the tls_idx_0 target
# mov [rip+0x11a5], eax at offset 0x2d
tls_idx_off = 0x13d8
print(f"\ntls_idx_0 at file offset 0x{tls_idx_off:x}:")
tls_idx = struct.unpack_from('<I', data, tls_idx_off)[0]
print(f"  Value: {tls_idx:#x}")

# state_ptr_0 is at 0x13dc (from 89 05 at 0x5c with disp 0x117a)
state_ptr_off = 0x13dc
print(f"\nstate_ptr_0 at file offset 0x{state_ptr_off:x}:")
state_ptr = struct.unpack_from('<Q', data, state_ptr_off)[0]
print(f"  Value: 0x{state_ptr:016x}")

# Let me also check what function the first FF 15 in DllMain calls
# call at 0x27: ff 15 e1 15 00 00 → disp=0x15e1, target=0x180e
# Let me verify all the IAT entries
print("\nAll IAT entries referenced from DllMain:")
for i in [0x27, 0x33, 0x4e, 0x72, 0x96, 0xa6, 0xbb, 0xd5]:
    disp = struct.unpack_from('<i', data, text_raw + i + 2)[0]
    target_off = text_raw + i + 6 + disp
    iat_val = struct.unpack_from('<Q', data, target_off)[0]
    hint_off = (iat_val - 0x1000) + text_raw if iat_val > 0x1000 else 0
    name = '?'
    if hint_off > 0 and hint_off < len(data) - 4:
        try:
            name_start = hint_off + 2
            name_end = data.index(b'\x00', name_start) if b'\x00' in data[name_start:name_start+50] else name_start+50
            name = data[name_start:name_end].decode('ascii', errors='replace')
        except: pass
    print(f"  FF 15 at 0x{i:x} → IAT 0x{target_off:x} = 0x{iat_val:016x} → '{name}'")
