"""Minimal PNG reader (standard library only), enough for tilesets.

    width, height, pixels = read_png(path)   # pixels: rows of (r, g, b, a)

Supports non-interlaced PNGs: greyscale, RGB, palette (1/2/4/8-bit), grey +
alpha and RGBA, 8 bits per channel, with tRNS transparency.
"""
import struct
import zlib


def _paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def read_png(path):
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path} is not a PNG file")
    pos, idat, palette, trns = 8, b"", [], None
    while pos < len(data):
        length, = struct.unpack(">I", data[pos:pos + 4])
        kind = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            w, h, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"PLTE":
            palette = [tuple(body[i:i + 3]) for i in range(0, len(body), 3)]
        elif kind == b"tRNS":
            trns = body
        elif kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            break
    if interlace:
        raise ValueError("interlaced PNGs aren't supported; re-save without interlacing")
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    if ctype != 3 and depth != 8:
        raise ValueError("only 8-bit channels are supported (or palette images)")
    bits = depth * channels
    stride = (w * bits + 7) // 8
    bpp = max(1, bits // 8)
    raw = zlib.decompress(idat)
    rows, prev = [], bytearray(stride)
    for y in range(h):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                line[i] = (line[i] + _paeth(a, b, c)) & 255
        rows.append(line)
        prev = line

    pixels = []
    for line in rows:
        out = []
        for x in range(w):
            if ctype == 3:
                per = 8 // depth
                byte = line[x // per]
                idx = (byte >> (8 - depth * (x % per + 1))) & ((1 << depth) - 1)
                r, g, b = palette[idx]
                a = trns[idx] if trns and idx < len(trns) else 255
            elif ctype == 0:
                r = g = b = line[x]
                a = 0 if trns and struct.unpack(">H", trns[:2])[0] == r else 255
            elif ctype == 4:
                r = g = b = line[2 * x]
                a = line[2 * x + 1]
            elif ctype == 2:
                r, g, b = line[3 * x:3 * x + 3]
                a = 255
                if trns and struct.unpack(">HHH", trns[:6]) == (r, g, b):
                    a = 0
            else:
                r, g, b, a = line[4 * x:4 * x + 4]
            out.append((r, g, b, a))
        pixels.append(out)
    return w, h, pixels
