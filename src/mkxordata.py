#!/usr/bin/python3

import zipfile

def lookahead(iterable):
    it = iter(iterable)
    try:
        last = next(it)
    except StopIteration:
        return

    for current in it:
        yield last, False
        last = current
    yield last, True


print(r'''/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include "xordata.h"

const XorDataMap xorDataMap = {''')

with open("xordata.lst", "rb") as f:
    b = f.readlines()

i = iter(b)

for (aname, bname), is_last_file in lookahead(zip(i, i)):
    aname = aname.decode().rstrip()
    bname = bname.decode().rstrip()

    print(" " * 4 + "{")
    print(" " * 8 + '"' + aname + '",')
    print(" " * 8 + "{")
    print(" " * 12 + '.name = "' + bname + '",')
    print(" " * 12 + '.contents = {')
    print(" " * 16, end="")

    try:
        with open(aname, "rb") as afile:
            adata = afile.read()
    except FileNotFoundError:
        adata = b'\x00\x55\xAA\xFF'*256+b'\x33\x66\x99'

    with zipfile.ZipFile("ultima4.zip", "r") as zfile:
        bdata = zfile.read(bname.upper())

    if len(bdata) < 1:
        raise RuntimeError("bdata has zero length")
    bdata_copy = bdata
    while len(bdata) < len(adata):
        bdata += bdata_copy

    for (num, (a, b)), is_last_byte in lookahead(enumerate(zip(adata, bdata), start=1)):
        print(f"0x{a ^ b:02x}", end="")
        if is_last_byte:
            print()
        else:
            if num % 8:
                print(", ", end="")
            else:
                print(",")
                print(" " * 16, end="")
    print(" " * 12 + "}")
    print(" " * 8 + "}")
    print(" " * 4 + "}", end="")
    if is_last_file:
        print()
    else:
        print(",")
print("};")
