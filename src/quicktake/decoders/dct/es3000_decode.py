#!/usr/bin/env python3
"""
es3000_decode.py - decoder for Chinon ES-3000 ".dct" image files

Reverse-engineered from the Amiga "ES3000 V1.2 (1.9.96)" demo by Vision
Factory Development (tables pulled from the es3000.fpu binary) and verified
against the two sample files shipped with it (Allen.dct, Tina.dct).

Usage:  python3 es3000_decode.py file.dct [more.dct ...]   -> writes file.png

FILE FORMAT (.dct as saved by the Amiga tool)
------------------------------------------------------------------------
0x000  4   "Cdcx" magic
0x004  4   00 00 00 02 (version?)
0x008  4   payload length, little-endian (24000 = Normal, 192000 = Super Fine)
0x00C  4   LE value, 1 or 2 in samples (unknown)
0x080..0x17F  uninitialised memory in the samples - ignore
0x180  ~28 camera picture-info record (little-endian; payload length again
           at 0x186; other fields not yet identified)
0x200  N   payload = raw data as sent by the camera

PAYLOAD: fixed-rate 8x8 DCT, no entropy coding
  Three planes, one after another:  Y blocks, then Cr blocks, then Cb blocks.
  Every block of a plane occupies the same number of bytes. Blocks are in
  raster order, BW blocks per row.

  mode        payload  Y blocks (bytes)   Cr/Cb blocks each (bytes)  luma size
  Normal 320   24000   20x30 (24 = 192b)  20x15 (16 = 128b)          160x240
  SuperFine    192000  40x60 (48 = 384b)  40x30 (32 = 256b)          320x480
  Fine 640     96000?  not seen - probably Normal tables at SF geometry

  Inside a block: a little-endian bitstream (LSB of byte 0 first). Walk the
  64 coefficients in JPEG zigzag order; coefficient at raster position r has
  BITS[r] bits (0 = not stored, value 0). Values are two's complement,
  except the luma DC which is unsigned. Dequantise with value << SHIFT[r].
  Then do a standard orthonormal 8x8 IDCT (DC = 8 * block mean).
  Unused trailing bits are zero.

  Luma is sampled at half the horizontal output resolution (non-square
  pixels), chroma at half of that vertically too: scale Y by 2x horizontally,
  Cr/Cb by 2x in both directions (relative to 8x8 blocks), then convert
  YCbCr -> RGB with the usual CCIR-601 coefficients (no +128 offset on
  chroma; chroma is signed).
"""
import sys
import numpy as np
from PIL import Image

def _t(s):
    return np.array(s.split(), dtype=np.int64)

# JPEG zigzag: raster position -> scan index (binary: table at code 0xb226, 1-based)
ZIGZAG = _t("""0 1 5 6 14 15 27 28 2 4 7 13 16 26 29 42 3 8 12 17 25 30 41 43
 9 11 18 24 31 40 44 53 10 19 23 32 39 45 52 54 20 22 33 38 46 51 55 60
 21 34 37 47 50 56 59 61 35 36 48 49 57 58 62 63""")
SCAN = np.argsort(ZIGZAG)            # scan index -> raster position

# (bit allocation, dequant shift), raster order. Offsets are in es3000.fpu.
TABLES = {
 'N_Y': (_t("""8 8 7 7 7 7 6 6 8 7 7 7 6 5 5 4 7 7 6 5 5 4 4 0 7 5 5 5 4 4 0 0
               6 5 0 0 0 0 0 0 5 0 0 0 0 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"""),  # file 0x38a84
         _t("""3 3 3 3 3 3 3 3 3 3 3 3 3 4 4 4 3 3 3 4 4 4 4 8 3 4 4 4 4 4 8 8
               3 4 8 8 8 8 8 7 3 8 8 7 7 7 7 7 4 7 6 6 6 6 6 6 6 6 6 6 6 6 6 6""")), # file 0x38a04
 'N_C': (_t("""9 8 6 4 3 3 3 3 7 6 4 3 3 3 3 0 6 4 4 3 3 3 0 0 4 4 3 3 3 0 0 0
               4 3 3 0 0 0 0 0 4 3 0 0 0 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"""),  # file 0xb41a
         _t("""2 2 2 3 3 3 3 3 2 2 3 3 3 3 3 6 2 3 3 3 3 3 6 6 3 3 3 3 3 6 6 6
               3 3 3 6 6 6 6 6 3 3 6 6 6 6 6 6 3 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6""")), # file 0xb39a
 'S_Y': (_t("""10 10 9 8 8 8 7 7 10 9 8 8 7 7 7 6 9 8 7 7 7 6 6 6 8 7 7 7 6 6 6 6
               7 7 6 6 6 6 6 5 6 6 6 5 5 5 4 4 5 5 4 4 3 3 3 3 4 4 3 3 3 3 3 3"""), # file 0x38b84
         _t("""1 1 1 2 2 2 2 2 1 1 2 2 2 2 2 2 1 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2
               2 2 2 2 2 2 2 2 2 2 2 2 2 2 3 3 2 2 2 2 3 3 3 3 2 2 3 3 3 3 3 3""")), # file 0x38b04
 'S_C': (_t("""10 10 7 6 5 4 4 4 9 7 6 4 4 4 4 4 7 6 5 4 4 4 3 3 5 5 4 4 3 3 3 3
               4 3 3 3 3 3 3 3 4 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3"""), # file 0x38c84
         _t("""1 0 1 1 1 2 2 2 0 1 1 2 2 2 2 2 1 1 2 2 2 2 3 3 2 2 2 2 3 3 3 3
               3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3""")), # file 0x38c04
}

# payload length -> (Y table, C table, Y bytes/block, C bytes/block, blocks per row, Y rows, C rows)
MODES = {
    24000:  ('N_Y', 'N_C', 24, 16, 20, 30, 15),   # Normal, 320x240 output (verified)
    192000: ('S_Y', 'S_C', 48, 32, 40, 60, 30),   # Super Fine, 640x480 output (verified)
    96000:  ('N_Y', 'N_C', 24, 16, 40, 60, 30),   # Fine, 640x480 - GUESS, no sample yet
}

# 8x8 orthonormal DCT basis (the Amiga code builds the same thing in Q10 fixed point)
_u = np.arange(8)[:, None]; _x = np.arange(8)[None, :]
BASIS = np.where(_u == 0, np.sqrt(1 / 8), 0.5) * np.cos((2 * _x + 1) * _u * np.pi / 16)


def decode_plane(data, nblocks, bpb, table, per_row, signed_dc):
    bits, shift = TABLES[table]
    order = [(int(r), int(bits[r]), int(shift[r])) for r in SCAN if bits[r]]
    rows = nblocks // per_row
    out = np.zeros((rows * 8, per_row * 8))
    for i in range(nblocks):
        v = int.from_bytes(data[i * bpb:(i + 1) * bpb], 'little')
        coef = np.zeros(64)
        pos = 0
        for k, (r, b, s) in enumerate(order):
            x = (v >> pos) & ((1 << b) - 1)
            pos += b
            if (k or signed_dc) and x >= 1 << (b - 1):
                x -= 1 << b
            coef[r] = x << s
        by, bx = divmod(i, per_row)
        out[by * 8:by * 8 + 8, bx * 8:bx * 8 + 8] = BASIS.T @ coef.reshape(8, 8) @ BASIS
    return out


def decode(path):
    raw = open(path, 'rb').read()
    if raw[:4] != b'Cdcx':
        raise ValueError('not a Cdcx file')
    n = int.from_bytes(raw[8:12], 'little')
    if n not in MODES:
        raise ValueError(f'unknown payload length {n}')
    ty, tc, yb, cb, bw, yr, cr = MODES[n]
    d = raw[512:512 + n]
    ny, nc = bw * yr, bw * cr
    Y  = decode_plane(d, ny, yb, ty, bw, False)
    Cr = decode_plane(d[ny * yb:], nc, cb, tc, bw, True)
    Cb = decode_plane(d[ny * yb + nc * cb:], nc, cb, tc, bw, True)

    W, H = Y.shape[1] * 2, Y.shape[0]
    up = lambda a: np.asarray(Image.fromarray(a.astype(np.float32)).resize((W, H), Image.BICUBIC))
    y, cb_, cr_ = up(Y), up(Cb), up(Cr)
    rgb = np.dstack([y + 1.402 * cr_, y - 0.344136 * cb_ - 0.714136 * cr_, y + 1.772 * cb_])
    return Image.fromarray(np.clip(rgb + 0.5, 0, 255).astype(np.uint8))


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for f in sys.argv[1:]:
        img = decode(f)
        out = f.rsplit('.', 1)[0] + '.png'
        img.save(out)
        print(f'{f} -> {out} ({img.width}x{img.height})')
