Serial debug console.
=====================
 
There are 3 functions for printing debug information on the serial
port.
 
 * ``debug`` - Print a debug message.
 * ``warn`` - Print a warning message starting with ``WARNING (filename):``
 * ``error`` - Print an error message starting with ``ERROR (filename):``

``debug`` and ``warn`` only invokes actual code when ``DEBUG`` and 
``WARNINGS`` are defined, otherwise no output is created.

WARNINGS
DEBUG
DEBUG_MEM
DEBUG_MEM_LIST
SDK_DEBUG
