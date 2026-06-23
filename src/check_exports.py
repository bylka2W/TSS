import ctypes
import ctypes.wintypes

d = ctypes.CDLL('C:/TSS/test_persist.dll')
k32 = ctypes.WinDLL('kernel32', use_last_error=True)
k32.GetModuleHandleW.argtypes = [ctypes.wintypes.LPCWSTR]
hm = k32.GetModuleHandleW('test_persist.dll')

def r32(addr):
    b = (ctypes.c_uint8*4)()
    ctypes.memmove(b, ctypes.c_void_p(addr), 4)
    return b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24)

def r16(addr):
    b = (ctypes.c_uint8*2)()
    ctypes.memmove(b, ctypes.c_void_p(addr), 2)
    return b[0]|(b[1]<<8)

def read_dos_header(hm):
    e_lfanew = r32(hm + 0x3C)
    print(f'e_lfanew = {hex(e_lfanew)}')
    return e_lfanew

def read_optional_header(hm, e_lfanew):
    opt_hdr = hm + e_lfanew + 24
    magic = r16(opt_hdr)
    print(f'Magic: {hex(magic)}')
    return opt_hdr, magic

def read_export_dir(hm, opt_hdr, magic):
    if magic == 0x20B:
        export_dir_rva = r32(opt_hdr + 112)
        export_dir_size = r32(opt_hdr + 116)
    else:
        export_dir_rva = r32(opt_hdr + 96)
        export_dir_size = r32(opt_hdr + 100)
    print(f'Export directory RVA: {hex(export_dir_rva)}, size: {export_dir_size}')
    return export_dir_rva

def dump_exports(hm, export_dir_rva):
    if not export_dir_rva:
        print('No export directory')
        return
    
    ed = hm + export_dir_rva
    n_functions = r32(ed + 20)
    n_names = r32(ed + 24)
    addr_of_fns = r32(ed + 28)
    addr_of_names = r32(ed + 32)
    addr_of_ordinals = r32(ed + 36)
    print(f'Functions: {n_functions}')
    print(f'Names: {n_names}')
    print(f'AddressOfFunctions RVA: {hex(addr_of_fns)}')
    
    fn_table = hm + addr_of_fns
    name_table = hm + addr_of_names
    ord_table = hm + addr_of_ordinals
    
    print()
    print('Functions:')
    for i in range(n_functions):
        fn_rva = r32(fn_table + i*4)
        print(f'  [{i}] RVA {hex(fn_rva)}')
    
    print()
    print('Names:')
    for i in range(n_names):
        name_rva = r32(name_table + i*4)
        ord = r16(ord_table + i*2)
        b = (ctypes.c_char * 64)()
        ctypes.memmove(b, ctypes.c_void_p(hm + name_rva), 64)
        name = b.value.split(b'\x00')[0].decode('ascii')
        fn_rva = r32(fn_table + ord*4)
        print(f'  [{i}] "{name}" ordinal={ord} fn_RVA={hex(fn_rva)}')
    
    # Now disasm function bytes
    print()
    print('Function code:')
    for i in range(n_functions):
        fn_rva = r32(fn_table + i*4)
        b = (ctypes.c_uint8 * 128)()
        ctypes.memmove(b, ctypes.c_void_p(hm + fn_rva), 128)
        print(f'  fn[{i}] at RVA {hex(fn_rva)}:')
        hexstr = ' '.join(f'{b[j]:02x}' for j in range(min(128, 80)))
        print(f'    {hexstr}')
        
        # Find the return value load - look for 41 8b 06 (mov eax, [r14])
        for j in range(120):
            if b[j] == 0x41 and b[j+1] == 0x8B and b[j+2] == 0x06:
                print(f'    -> mov eax, [r14] at offset {j}')
            if b[j] == 0x41 and b[j+1] == 0x8B and b[j+2] == 0x46 and b[j+3] == 0x00:
                print(f'    -> mov eax, [r14+0] at offset {j}')

e_lfanew = read_dos_header(hm)
opt_hdr, magic = read_optional_header(hm, e_lfanew)
export_rva = read_export_dir(hm, opt_hdr, magic)
dump_exports(hm, export_rva)
