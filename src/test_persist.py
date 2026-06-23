import ctypes
d = ctypes.CDLL('C:/TSS/test_persist.dll')
d.set.restype = ctypes.c_int64
d.get.restype = ctypes.c_int64

v1 = d.get()
print(f'get before set: {v1}')

v2 = d.set()
print(f'set: {v2}')

v3 = d.get()
print(f'get after set: {v3}')

# Second round: should return 42 again (persisted)
v4 = d.get()
print(f'get again: {v4}')

# Overwrite
d.set()
d.get()
# After set+get, should be 42
# Now check chained
d.set()
v5 = d.get()
print(f'get after chained set: {v5}')
