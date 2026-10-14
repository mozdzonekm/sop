#!/usr/bin/env python3
"""Generates a sample `database_inventory` file for Etap 4.

Record format (68 bytes per entry):
  * 4  bytes : size of the ingredient file (unsigned int, little-endian)
  * 64 bytes : name from the metadata (NUL-padded if shorter than 64 bytes)

Entries are built from the real files in the warehouse directory, plus two
intentionally wrong entries so that all Etap 4 branches can be tested:
  * "Miecz Trola"      -> wrong size in the database (size mismatch case)
  * "Phial of Eternity"-> does not exist in the warehouse (missing case)
"""

import os
import struct
import sys

WAREHOUSE = "warehouse"
OUT = "database_inventory"
RECORD = struct.Struct("<I64s")  # 4-byte uint + 64-byte name field = 68 bytes


def parse_name(path):
    """Return the value of the `name:` field from a metadata file."""
    with open(path, encoding="utf-8") as f:
        for line in f:
            if ":" in line:
                key, _, value = line.partition(":")
                if key.strip() == "name":
                    return value.strip()
    return None


def walk_files():
    files = []
    for root, _dirs, names in os.walk(WAREHOUSE):
        for n in names:
            p = os.path.join(root, n)
            if os.path.isfile(p) and not os.path.islink(p):
                files.append(p)
    return sorted(files)


def main():
    entries = []
    for p in walk_files():
        name = parse_name(p)
        if name is None:
            print(f"warning: no name field in {p}, skipping", file=sys.stderr)
            continue
        size = os.stat(p).st_size
        # Intentional error: wrong size for this ingredient.
        if name == "Miecz Trola":
            size = 999
        entries.append((size, name))

    # Intentional error: this ingredient is not present in the warehouse.
    entries.append((50, "Phial of Eternity"))

    with open(OUT, "wb") as f:
        for size, name in entries:
            f.write(RECORD.pack(size, name.encode("utf-8")[:64]))

    print(f"Wrote {OUT}: {len(entries)} entries, {len(entries) * 68} bytes")


if __name__ == "__main__":
    main()
