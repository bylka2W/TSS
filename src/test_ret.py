import ctypes, struct
d = ctypes.CDLL('C:/TSS/test_ret_float.dll')
for name in ['set_float_a', 'set_float_b', 'set_float_c', 'set_float_add']:
    f = getattr(d, name)
    f.restype = ctypes.c_int64
    v = f()
    as_float = struct.unpack('f', struct.pack('I', v & 0xFFFFFFFF))[0]
    print(f'{name}: u32={v & 0xFFFFFFFF:#010x} ({v & 0xFFFFFFFF}), float={as_float}')
# Also test init alone
d.init.restype = ctypes.c_int64
vi = d.init()
print(f'init: {vi}')
# Test chained: init then set_float_a then set_float_add
d.init()
va = d.set_float_a()
print(f'set_float_a after init: {va:#010x}')
vadd = d.set_float_add()
print(f'set_float_add after set_float_a (expected 1.5+1=2.5): {vadd:#010x}')
