param($dllPath)
$data = [System.IO.File]::ReadAllBytes($dllPath)
$pe = [System.BitConverter]::ToUInt32($data, 0x3C)
$opt = $pe + 24
$numDirs = [System.BitConverter]::ToUInt32($data, $opt + 0x6C)
Write-Host "DataDirs: $numDirs"
$dir = $opt + 0x70
$impRva = [System.BitConverter]::ToUInt32($data, $dir + 8)
$impSz = [System.BitConverter]::ToUInt32($data, $dir + 12)
Write-Host ("Import RVA=0x{0:X} Sz=0x{1:X}" -f $impRva, $impSz)
$sectionOff = 0x188
$sectVRva = [System.BitConverter]::ToUInt32($data, $sectionOff + 12)
$sectRaw = [System.BitConverter]::ToUInt32($data, $sectionOff + 20)
Write-Host ("Section VRVA=0x{0:X} RawPtr=0x{1:X}" -f $sectVRva, $sectRaw)
$impFileOff = $impRva - $sectVRva + $sectRaw
Write-Host ("Import at file offset 0x{0:X}" -f $impFileOff)
# Dump string table area
for ($i = 0; $i -lt 128; $i++) {
    $b = $data[$impFileOff + $i]
    $c = if ($b -ge 32 -and $b -lt 127) { [char]$b } else { '.' }
    Write-Host ("{0:X4}: {1:X2} ({2})" -f ($impFileOff+$i), $b, $c)
}
