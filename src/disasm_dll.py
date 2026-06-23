import struct, sys

def read_pe_exports(path):
    with open(path, 'rb') as f:
        data = f.read()
    
    # Find PE signature
    pe_sig_off = struct.unpack_from('<I', data, 0x3C)[0]
    if data[pe_sig_off:pe_sig_off+4] != b'PE\x00\x00':
        print("Invalid PE")
        return
    
    # Get section headers
    num_sections = struct.unpack_from('<H', data, pe_sig_off + 6)[0]
    opt_hdr_size = struct.unpack_from('<H', data, pe_sig_off + 20)[0]
    first_section = pe_sig_off + 24 + opt_hdr_size
    
    # Find .text section
    text_rva = 0
    text_size = 0
    text_raw = 0
    for i in range(num_sections):
        s_off = first_section + i * 40
        name = data[s_off:s_off+8].rstrip(b'\x00').decode()
        if name == '.text':
            text_rva = struct.unpack_from('<I', data, s_off + 12)[0]
            text_size = struct.unpack_from('<I', data, s_off + 8)[0]
            text_raw = struct.unpack_from('<I', data, s_off + 20)[0]
            break
    
    print(f".text: rva=0x{text_rva:x}, size=0x{text_size:x}, raw=0x{text_raw:x}")
    
    # Find export directory
    data_dir_rva = struct.unpack_from('<I', data, pe_sig_off + 24 + 112)[0]
    data_dir_size = struct.unpack_from('<I', data, pe_sig_off + 24 + 116)[0]
    print(f"Export dir: rva=0x{data_dir_rva:x}, size=0x{data_dir_size:x}")
    
    # Convert export dir RVA to file offset
    export_off = data_dir_rva - text_rva + text_raw
    
    if export_off >= len(data):
        print("Export dir not in .text")
        return
    
    # Parse export directory
    n_fns = struct.unpack_from('<I', data, export_off + 20)[0]
    n_names = struct.unpack_from('<I', data, export_off + 24)[0]
    eat_rva = struct.unpack_from('<I', data, export_off + 28)[0]
    enpt_rva = struct.unpack_from('<I', data, export_off + 32)[0]
    eot_rva = struct.unpack_from('<I', data, export_off + 36)[0]
    
    eat_off = eat_rva - text_rva + text_raw
    enpt_off = enpt_rva - text_rva + text_raw
    eot_off = eot_rva - text_rva + text_raw
    
    print(f"Functions: {n_fns}, Names: {n_names}")
    print(f"EAT offset: 0x{eat_off:x}, ENPT offset: 0x{enpt_off:x}")
    
    # Get the names and ordinals
    exports = []
    for i in range(n_names):
        name_rva = struct.unpack_from('<I', data, enpt_off + i * 4)[0]
        name_off = name_rva - text_rva + text_raw
        name = data[name_off:data.index(b'\x00', name_off)].decode()
        ordinal = struct.unpack_from('<H', data, eot_off + i * 2)[0]
        func_rva = struct.unpack_from('<I', data, eat_off + ordinal * 4)[0]
        func_off = func_rva - text_rva + text_raw
        exports.append((name, ordinal, func_rva, func_off))
    
    # Print exports sorted by name
    for name, ordinal, func_rva, func_off in sorted(exports, key=lambda x: x[0]):
        print(f"  {name}: ordinal={ordinal}, rva=0x{func_rva:x}, off=0x{func_off:x}")
        
        # Print first 40 bytes of each export
        print(f"    Code bytes: {data[func_off:func_off+40].hex()}")
    
    return exports, data

if __name__ == '__main__':
    exports, data = read_pe_exports('C:/TSS/test_persist.dll')
    
    if exports and data:
        print("\n--- Finding tls_idx_0 and state_ptr_0 ---")
        # We know the code ends with embedStateGlobals + IAT
        # Let's search for the IAT pattern at the end of .text
        text_rva = 0x1000
        for name, ord, rva, off in exports:
            if name == 'set':
                set_bytes = data[off:off+80]
                print(f"\n'set' code bytes (80): {set_bytes.hex()}")
                # Look for 8B 0D (mov ecx, [rip+disp])
                for i in range(len(set_bytes)-5):
                    if set_bytes[i] == 0x8B and set_bytes[i+1] == 0x0D:
                        disp = struct.unpack_from('<i', set_bytes, i+2)[0]
                        target = (off + i + 6) + disp  # RIP-relative
                        print(f"  Found mov ecx,[rip+disp] at offset 0x{off+i:x}")
                        print(f"    disp = {disp:#x}, target file offset = 0x{target:x}")
                        if target < len(data):
                            val = struct.unpack_from('<I', data, target)[0]
                            print(f"    value at target = {val:#x} ({val})")
                # Look for FF 15 (call [rip+disp])
                for i in range(len(set_bytes)-5):
                    if set_bytes[i] == 0xFF and set_bytes[i+1] == 0x15:
                        disp = struct.unpack_from('<i', set_bytes, i+2)[0]
                        target = (off + i + 6) + disp
                        print(f"  Found call [rip+disp] at offset 0x{off+i:x}")
                        print(f"    disp = {disp:#x}, target file offset = 0x{target:x}")
                        if target < len(data):
                            val = struct.unpack_from('<Q', data, target)[0]
                            if val != 0:
                                # This is a VA, not a file offset
                                print(f"    VA at target = 0x{val:x}")
            elif name == 'get':
                get_bytes = data[off:off+80]
                print(f"\n'get' code bytes (80): {get_bytes.hex()}")
