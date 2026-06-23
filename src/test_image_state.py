import ctypes, struct

d = ctypes.CDLL('C:/TSS/src/test_image_state.dll')
for name in ['Init', 'GetHistory00', 'GetHistory01', 'WriteHistory', 'GetAccumColor']:
    getattr(d, name).restype = ctypes.c_int64

print('Init:', d.Init())
# history[0,0] = 42.0, bitcast as int64 = 0x4228000000000000 -> but c_int64 truncates to 32 bits on some platforms
# Actually MOVD puts 32-bit float in lower 32 bits of RAX
h00 = d.GetHistory00()
print(f'GetHistory00: {h00} (expect float 42.0 as bitcast int = {ctypes.c_int(struct.unpack("i", struct.pack("f", 42.0))[0]).value})')

h01 = d.GetHistory01()
print(f'GetHistory01: {h01} (expect float 0.0 as bitcast int = 0)')

w = d.WriteHistory()
print(f'WriteHistory: {w} (expect 43.0 as bitcast int = {ctypes.c_int(struct.unpack("i", struct.pack("f", 43.0))[0]).value})')

w2 = d.WriteHistory()
print(f'WriteHistory again: {w2} (expect 44.0)')

ac = d.GetAccumColor()
print(f'GetAccumColor: {ac} (expect float 0.5 as bitcast = {ctypes.c_int(struct.unpack("i", struct.pack("f", 0.5))[0]).value})')

# Read values interpreted as float
def read_float(dll, name):
    f = getattr(dll, name)
    f.restype = ctypes.c_float
    return f()

d2 = ctypes.CDLL('C:/TSS/src/test_image_state.dll')
print()
print('As floats:')
print(f'  GetHistory00: {read_float(d2, "GetHistory00"):.6f}')
print(f'  GetHistory01: {read_float(d2, "GetHistory01"):.6f}')
print(f'  WriteHistory: {read_float(d2, "WriteHistory"):.6f}')
print(f'  GetAccumColor: {read_float(d2, "GetAccumColor"):.6f}')
