# Normalize the non-deterministic PE fields so the driver build is byte-reproducible:
# zero the COFF TimeDateStamp and the Debug Directory timestamp, then recompute the PE
# checksum LAST over the now-deterministic bytes. The checksum is recomputed, not zeroed,
# because link /release sets it so a driver carries a valid checksum, and zeroing it discarded
# that (call/0024). Recomputing over already-normalized content keeps the build byte-identical
# AND restores the /release checksum.
import sys, struct
p = sys.argv[1]; d = bytearray(open(p,'rb').read())
pe = struct.unpack_from('<I', d, 0x3c)[0]
struct.pack_into('<I', d, pe+8, 0)                       # COFF TimeDateStamp
opt = pe + 24
magic = struct.unpack_from('<H', d, opt)[0]
ckoff = opt + 64                                         # CheckSum (PE32)
struct.pack_into('<I', d, ckoff, 0)
# Debug Directory = data directory index 6
ddoff = opt + (96 if magic == 0x10b else 112)
dbg_rva, dbg_sz = struct.unpack_from('<II', d, ddoff + 6*8)
if dbg_rva and dbg_sz:
    # map RVA->file offset via section table
    nsec = struct.unpack_from('<H', d, pe+6)[0]
    sh = opt + struct.unpack_from('<H', d, pe+20)[0]
    def rva2off(r):
        for i in range(nsec):
            b = sh + i*40
            va = struct.unpack_from('<I', d, b+12)[0]; vs = struct.unpack_from('<I', d, b+8)[0]
            ptr = struct.unpack_from('<I', d, b+20)[0]
            if va <= r < va+max(vs,1): return ptr + (r-va)
        return None
    off = rva2off(dbg_rva)
    if off:
        for k in range(0, dbg_sz, 28):                   # each IMAGE_DEBUG_DIRECTORY
            struct.pack_into('<I', d, off+k+4, 0)         # TimeDateStamp field
# Recompute the PE checksum last, over the fully-normalized (deterministic) bytes. The CheckSum
# field is already zeroed above, so summing the whole file treats it as zero, matching the
# imagehlp CheckSumMappedFile algorithm the loader validates. Deterministic input -> identical
# checksum every build, so byte-reproducibility is preserved and the /release checksum is valid.
n = len(d); full = n - (n & 3); s = 0
for dw in struct.unpack('<%dI' % (full // 4), bytes(d[:full])):
    s += dw; s = (s & 0xffffffff) + (s >> 32)
if n & 3:
    s += struct.unpack('<I', bytes(d[full:]) + b'\x00' * (4 - (n & 3)))[0]
    s = (s & 0xffffffff) + (s >> 32)
s = (s & 0xffff) + (s >> 16); s = s + (s >> 16); s &= 0xffff
struct.pack_into('<I', d, ckoff, (s + n) & 0xffffffff)
open(p,'wb').write(d)
