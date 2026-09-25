#!/usr/bin/python3

import os
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
    b_byte = f.readlines()

i = iter(b_byte)

for (a_name, b_name), is_last_file in lookahead(zip(i, i)):
    a_name = a_name.decode().rstrip()
    b_name = b_name.decode().rstrip()

    print(" " * 4 + "{")
    print(" " * 8 + '"' + a_name + '",')
    print(" " * 8 + "{")
    print(" " * 12 + '.name = "' + b_name + '",')
    print(" " * 12 + '.contents = {')
    print(" " * 16, end="")

    with open("out" + os.sep + a_name, "rb") as a_file:
        a_data = a_file.read()

    with zipfile.ZipFile("ultima4.zip", "r") as b_file:
        b_data = b_file.read(b_name.upper())
    if len(b_data) < 1:
        raise RuntimeError("bdata has zero length")
    b_data_copy = b_data
    while len(b_data) < len(a_data):
        b_data += b_data_copy

    for (num, (a_byte, b_byte)), is_last_byte in lookahead(
            enumerate(zip(a_data, b_data), start=1)
    ):
        print(f"0x{a_byte ^ b_byte:02x}", end="")
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
