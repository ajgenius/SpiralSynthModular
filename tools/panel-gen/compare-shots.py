#!/usr/bin/env python3
import os
import struct
import sys
import zlib


def png_pixels(path):
    data = open(path, "rb").read()
    pos = 8
    idat = b""
    w = h = ct = 0
    while pos < len(data):
        ln = struct.unpack(">I", data[pos:pos + 4])[0]
        typ = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + ln]
        pos += 12 + ln
        if typ == b"IHDR":
            w, h, _, ct = struct.unpack(">IIBB", body[:10])
        elif typ == b"IDAT":
            idat += body
    raw = zlib.decompress(idat)
    bpp = {2: 3, 6: 4, 0: 1}[ct]
    stride = w * bpp
    rows = []
    prev = bytearray(stride)
    p = 0
    for _ in range(h):
        f = raw[p]
        p += 1
        line = bytearray(raw[p:p + stride])
        p += stride
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
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 255
        rows.append(bytes(line))
        prev = line
    return w, h, bpp, rows


def compare(editor, panel):
    ew, eh, eb, er = png_pixels(editor)
    pw, ph, pb, pr = png_pixels(panel)
    if (ew, eh) != (pw, ph):
        return (ew, eh), (pw, ph), None
    diff = 0
    for y in range(eh):
        a, b = er[y], pr[y]
        for x in range(ew):
            if a[x * eb:x * eb + 3] != b[x * pb:x * pb + 3]:
                diff += 1
    return (ew, eh), (pw, ph), 100.0 * diff / (ew * eh)


def main():
    shots = sys.argv[1] if len(sys.argv) > 1 else os.path.expanduser("~/.hermes/cache/scratch/shots")
    rows = []
    for f in sorted(os.listdir(shots)):
        if not f.endswith("-editor.png"):
            continue
        name = f[:-len("-editor.png")]
        panel = os.path.join(shots, name + "-panel.png")
        if not os.path.exists(panel):
            rows.append((name, None, None, None))
            continue
        rows.append((name,) + compare(os.path.join(shots, f), panel))
    rows.sort(key=lambda r: (-1 if r[3] is None else -r[3]))
    for name, es, ps, pct in rows:
        if es is None:
            print("%-16s no panel shot" % name)
        elif pct is None:
            print("%-16s editor %dx%d  panel %dx%d  (size differs)" % (name, es[0], es[1], ps[0], ps[1]))
        else:
            print("%-16s %dx%d  %5.1f%% differ" % (name, es[0], es[1], pct))


if __name__ == "__main__":
    main()
