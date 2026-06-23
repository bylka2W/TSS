import ctypes, struct

d = ctypes.CDLL('C:/TSS/src/test_image_state.dll')

d.bpc_enum_fields.restype = ctypes.c_int64
table = d.bpc_enum_fields()
print(f'Field table ptr: {table:#x}')

FIELD_RECORD_SIZE = 48  # 32 name + 4 type + 4 reserved + 8 offset

i = 0
while True:
    offset = table + i * FIELD_RECORD_SIZE
    name_bytes = ctypes.string_at(offset, 32)
    name = name_bytes.split(b'\x00')[0].decode()
    if len(name) == 0:
        break
    type_tag = ctypes.c_uint32.from_address(offset + 32).value
    type_names = {0: 'INT', 1: 'FLOAT', 2: 'IMAGE_F32'}
    off = ctypes.c_int64.from_address(offset + 40).value
    print(f'  [{i}] "{name}" type={type_names.get(type_tag, str(type_tag))} offset={off}')
    i += 1
print(f'Total {i} fields')

d.Init.restype = ctypes.c_int64
d.Init()

d.bpc_get_state.restype = ctypes.c_int64
ptr = d.bpc_get_state()
print(f'State ptr: {ptr:#x}')

# Read accum_color (float at offset 0)
accum = struct.unpack('f', ctypes.string_at(ptr, 4))[0]
print(f'accum_color (offset 0): {accum}')

# Read frame_index (int32 at offset 16)
frame_idx = ctypes.c_int32.from_address(ptr + 16).value
print(f'frame_index (offset 16): {frame_idx}')

# Read result (int32 at offset 20)
result_val = ctypes.c_int32.from_address(ptr + 20).value
print(f'result (offset 20): {result_val}')

# Read history pointer (int64 at offset 24)
hist_ptr = ctypes.c_int64.from_address(ptr + 24).value
print(f'history ptr (offset 24): {hist_ptr:#x}')
if hist_ptr:
    hist_stride = ctypes.c_uint32.from_address(hist_ptr).value
    hist_width = ctypes.c_uint32.from_address(hist_ptr + 4).value
    hist_height = ctypes.c_uint32.from_address(hist_ptr + 8).value
    print(f'  stride={hist_stride} width={hist_width} height={hist_height}')
    pixel = struct.unpack('f', ctypes.string_at(hist_ptr + 16, 4))[0]
    print(f'  history[0,0] = {pixel}')
