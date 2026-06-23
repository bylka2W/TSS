import struct

with open('C:/TSS/test_persist.dll', 'rb') as f:
    data = f.read()

# .text section: raw=0x200, RVA=0x1000, size=0x1682
# DllMain should be at the start of .text: file offset 0x200
text_raw = 0x200
code = data[text_raw:text_raw+0x200]

# DllMain is at the beginning of .text
print("DllMain code (first 200 bytes):")
for i in range(0, min(0xC0, len(code)), 16):
    hex_str = ' '.join(f'{b:02x}' for b in code[i:i+16])
    print(f"  0x{i:04x}: {hex_str}")

# Look for TlsAlloc call and the store that follows
# TlsAlloc = index 28 → iat_28 label
# emitIatCall: FF 15 xx xx xx xx
print("\nSearching for DllMain patterns:")
# Find the first FF 15 after start
for i in range(len(code)-5):
    if code[i] == 0xFF and code[i+1] == 0x15:
        disp = struct.unpack_from('<i', code, i+2)[0]
        target = (text_raw + i + 6) + disp
        # Read the 8-byte IAT entry at the target
        if target < len(data):
            iat_val = struct.unpack_from('<Q', data, target)[0]
            print(f"  FF 15 at 0x{i:x}: disp={disp:#x}, target=0x{target:x}, IAT_val=0x{iat_val:x}")
            # Check if this is TlsAlloc by looking at what follows
            # TlsAlloc return in eax should be followed by a store

# Also find the first 89 05 (mov [rip+disp], eax) which would be emitRipRelativeStore32
print("\nSearching for emitRipRelativeStore32 (89 05):")
for i in range(len(code)-5):
    if code[i] == 0x89 and code[i+1] == 0x05:
        disp = struct.unpack_from('<i', code, i+2)[0]
        target = (text_raw + i + 6) + disp
        print(f"  89 05 at 0x{i:x}: disp={disp:#x}, target=0x{target:x}")

# Also search for the emitRipRelativeLoad32 used in export stubs
print("\nSearching for 8B 0D (mov ecx, [rip+disp]):")
for i in range(len(code)-5):
    if code[i] == 0x8B and code[i+1] == 0x0D:
        disp = struct.unpack_from('<i', code, i+2)[0]
        target_off = text_raw + i + 6 + disp
        print(f"  8B 0D at 0x{i:x}: disp={disp:#x}, target_file_off=0x{target_off:x}")
        if target_off < len(data):
            val = struct.unpack_from('<I', data, target_off)[0]
            print(f"    value at target = {val:#x}")
