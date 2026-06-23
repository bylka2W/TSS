import ctypes, struct

# Test FSR DLL with layout engine
d = ctypes.CDLL('C:/TSS/tss_fsr.dll')
for name in ['FrameInit', 'FrameProcess', 'GetFrameIndex', 'GetAccumWeightInt', 'GetAccumColorInt']:
    f = getattr(d, name)
    f.restype = ctypes.c_int64

d.FrameInit.restype = ctypes.c_int64
d.FrameProcess.restype = ctypes.c_int64
d.GetFrameIndex.restype = ctypes.c_int64
d.GetAccumWeightInt.restype = ctypes.c_int64
d.GetAccumColorInt.restype = ctypes.c_int64

# Sequential test
print('FSR pipeline:')
print(f'FrameInit = {d.FrameInit()}')
for i in range(5):
    d.FrameProcess()
    print(f'  Frame {i+1}: idx={d.GetFrameIndex()}, wt={d.GetAccumWeightInt()}, clr={d.GetAccumColorInt()}')
