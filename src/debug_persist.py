import ctypes
import ctypes.wintypes

d = ctypes.CDLL('C:/TSS/test_persist.dll')

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
k32.GetModuleHandleW.argtypes = [ctypes.wintypes.LPCWSTR]
hm = k32.GetModuleHandleW('test_persist.dll')
print(f'base = {hex(hm)}')

def read_u32(addr):
    b = (ctypes.c_uint8 * 4)()
    ctypes.memmove(b, ctypes.c_void_p(addr), 4)
    return b[0] | (b[1]<<8) | (b[2]<<16) | (b[3]<<24)

def read_u64(addr):
    b = (ctypes.c_uint8 * 8)()
    ctypes.memmove(b, ctypes.c_void_p(addr), 8)
    return b[0] | (b[1]<<8) | (b[2]<<16) | (b[3]<<24) | (b[4]<<32) | (b[5]<<40) | (b[6]<<48) | (b[7]<<56)

tls_idx = read_u32(hm + 0x21D8)
state_ptr = read_u64(hm + 0x21DC)
print(f'tls_idx_0 = {tls_idx} (0x{tls_idx:x})')
print(f'state_ptr_0 = 0x{state_ptr:x}')

# read heap block
hb = read_u64(state_ptr)  # first 8 bytes
print(f'Heap block [0:8] = 0x{hb:x} ({hb})')

# set
d.set.argtypes = []
d.set.restype = ctypes.c_int
sv = d.set()
print(f'set() = {sv}')
hb = read_u64(state_ptr)
print(f'Heap block [0:8] after set = 0x{hb:x} ({hb})')

# get
d.get.argtypes = []
d.get.restype = ctypes.c_int
gv = d.get()
print(f'get() = {gv}')
hb = read_u64(state_ptr)
print(f'Heap block [0:8] after get = 0x{hb:x} ({hb})')

# now let's also check: what is the R14 value during get?
# We can check by reading [hm + RVA of get export body where it reads [R14]]
# Actually, let's also try calling set then get repeatedly 
