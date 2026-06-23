import ctypes, ctypes.wintypes
k32 = ctypes.WinDLL('kernel32', use_last_error=True)
k32.GetModuleHandleW.argtypes = [ctypes.wintypes.LPCWSTR]
d = ctypes.CDLL('C:/TSS/test_ret_float.dll')
hm = k32.GetModuleHandleW('test_ret_float.dll')

def r32(a):
    return ctypes.c_uint32.from_address(a).value

def r16(a):
    return ctypes.c_uint16.from_address(a).value

e_lfanew = r32(hm + 0x3C)
opt_hdr = hm + e_lfanew + 24
magic = r16(opt_hdr)
if magic == 0x20B:
    export_rva = r32(opt_hdr + 112)
else:
    export_rva = r32(opt_hdr + 96)

ed = hm + export_rva
n_fn = r32(ed + 20)
n_nm = r32(ed + 24)
fn_a = r32(ed + 28)
nm_a = r32(ed + 32)
ord_a = r32(ed + 36)
print(f'nFunctions={n_fn}, nNames={n_nm}')
print(f'fnRVA=0x{fn_a:x}, nmRVA=0x{nm_a:x}')

for i in range(n_fn):
    fn_rva = r32(hm + fn_a + i*4)
    print(f'  fn[{i}] RVA 0x{fn_rva:x}')

for i in range(n_nm):
    name_rva = r32(hm + nm_a + i*4)
    ord_val = r16(hm + ord_a + i*2)
    b = (ctypes.c_char * 64)()
    ctypes.memmove(b, ctypes.c_void_p(hm + name_rva), 64)
    name = b.value.split(b'\x00')[0].decode('ascii')
    fn_rva = r32(hm + fn_a + ord_val * 4)
    print(f'  "{name}" ord={ord_val} RVA=0x{fn_rva:x}')

d.test_ret_float.argtypes = []
d.test_ret_float.restype = ctypes.c_float
print(f'result: {d.test_ret_float():.6f}')
