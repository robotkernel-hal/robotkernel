===============
Getting started
===============

This section describes how to install and use robotkernel on your linux 
system. For RM linux systems and debian/ubuntu there's an official 
repository available. For all other you will need to build robotkernel
from source.

First steps on RM linux systems with CISSy
==========================================

To test if robotkernel is available on your linux machine just try to
install and make a test run with conan::

  conan install --requires=robotkernel/[~6]@robotkernel/stable -of conan -pr:a $DLRRM_HOST_PLATFORM
  bash -c "source conan/conanrun.sh; robotkernel"

This should output robotkernel's help page::

  2026-05-12 08:21:49.995 INFO [robotkernel|robotker] usage: robotkernel --config | -c <filename> [--quiet, -q] [--verbose, -v] [--power_up, -p module=[boot,init,preop,safeop,op] [--help, -h]
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker]
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker]   --config, -c <filename>     specify config file
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker]   --quiet, -q                 run in quiet mode
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker]   --verbose, -v               be more verbose
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker]   --help, -h                  this help page
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker]   --test-run, -t              doing test run, load modules and quit
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker]   --power_up, -p              powering up modules
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker] exiting
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker] destructing...
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker] removing modules
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker] removing bridges
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker] removing service providers
  2026-05-12 08:21:49.996 INFO [robotkernel|robotker] clean up finished

If you see this output everything should be fine and you can continue
with the :doc:`configuration` section.

Install on debian/ubuntu
========================

Please refer to the open-source project https://robotkernel.org.

Build from source
=================

TBD

