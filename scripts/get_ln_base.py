#!/usr/bin/python

import os
import sys
import subprocess

pkgtool = "/volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/pkgtool"

for arg in sys.argv[1:]:
    entries = arg.split(',')

    for entry in entries:
        if entry.strip().startswith("software.links_and_nodes"):
            if os.path.isfile(pkgtool):
                p = subprocess.Popen([pkgtool, "--key", "PKGROOT", entry.strip()])
                p.wait()
            else:
                print os.getenv("LN_BASE")

