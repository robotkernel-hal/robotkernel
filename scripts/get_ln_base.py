#!/usr/bin/python

import sys
import subprocess

for arg in sys.argv[1:]:
    entries = arg.split(',')

    for entry in entries:
        if entry.strip().startswith("software.links_and_nodes"):
            p = subprocess.Popen(["/volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/pkgtool", "--key", "PKGROOT", entry.strip()])
            p.wait()

