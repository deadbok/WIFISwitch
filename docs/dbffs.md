DBF File System.
================

Simple read-only file system, created for use with the esp8266.

Features.
---------

 * Simple read-only file system.
 * Links, much like symbolic links on UNIX.

Structure.
----------

The file system is an array of headers, and starts with a file system
signature (0xDBFF5000) 4 bytes long.

	
    ---------------------
    |    FS signature   |
    ---------------------
    |       Header      |
    |...................-
    | File data or link |
    |                   |
    ~~~~~~~~~~~~~~~~~~~~~
    ~~~~~~~~~~~~~~~~~~~~~
    |       Header      |
    |...................-
    | File data or link |
    |                   |
    ---------------------


### Headers. ###


Each header starts with a 4 byte signature, to identify the type. There
are 2 types of entries.
 
 * Files (0xDBFF500F).
 * Links (0xDBFF5001).
 
#### File header. ####

 * Signature, 0xDBFF500F, 4 bytes.
 * Offset from this header to the next, 4 bytes.
 * Length of name, 1 byte.
 * Name, see above for size.
 * Size of file data in bytes, 4 bytes.
 * File data, see above for size.

#### Link header. ####

 * Signature, 0xDBFF5001, 4 bytes.
 * Offset from this header to the next, 4 bytes.
 * Length of name, 1 byte.
 * Name, see above for size.
 * Length of target path, 1 byte.
 * Target path. see above for size.
 
Limits.
-------

Entries are either files, or links.

 * 256 character entry name limit.
 * 4,294,967,295 bytes in file system.
 * No directories.
 
Even though directories are not supported at this time, file names may
contain slashes (``/``). This makes it possible to have the "feel" of
directory support without, the real deal.
 
ESP8266 firmware limits.
------------------------

The ESP8266 firmware can only resolve absolute symbolic links. The file
system supports relative path in links, but the firmware will fail.
 
Image creation.
---------------

`dbffs-image` is a tool to create a DBF file system image from a
directory tree. On systems supporting symbolic links, these are made
in to links on the target as well. There is no validation of the image,
and especially links are not checked in any way.
 
