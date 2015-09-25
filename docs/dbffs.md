DBF File System.
================

Simple read-only file system, created for use with the esp8266.

Features.
---------

 * Simple read-only file system.
 * Links, much like symbolic links on UNIX.

Structure.
----------

The file system is an array of headers, and always start with the header
of the root directory.
	
	---------------------
	| Directory header  |
	---------------------
	|       Header      |
	|...................-
	|   Possible file   |
	|        data.      |
	~~~~~~~~~~~~~~~~~~~~~
	~~~~~~~~~~~~~~~~~~~~~
	|       Header      |
	|...................|
	|   Possible file   |
	|        data.      |
	---------------------

### Headers. ###


Each header starts with a 4 byte signature, to identify the type. There
are 3 types of entries.
 
 * Files (0xDBFF500F).
 * Directories (0xDBFF500D).
 * Links (0xDBFF5001).
 
#### File header. ####

 * Signature, 0xDBFF500F, 4 bytes.
 * Offset from this header to the next, 4 bytes.
 * Length of name, 1 byte.
 * Name, see above for size.
 * Size of file data in bytes, 4 bytes.
 * File data, see above for size.

#### Directory header. ####

 * Signature, 0xDBFF500D, 4 bytes.
 * Offset from this header to the next, 4 bytes.
 * Length of name, 1 byte.
 * Name, see above for size.
 * Number of entries in directory, 2 bytes.

#### Link header. ####

 * Signature, 0xDBFF5001, 4 bytes.
 * Offset from this header to the next, 4 bytes.
 * Length of name, 1 byte.
 * Name, see above for size.
 * Length of target path, 1 byte.
 * Target path. see above for size.
 
Limits.
-------

Entries are either directories, files, or links.

 * 256 character entry name limit.
 * 65536 Entries in each directory.
 * 4,294,967,295 bytes in file system.
 
Image creation.
---------------

`dbffs-image` is a tool to create a file DBF file system image from a
directory tree. On systems supporting symbolic links, these are made
in to links on the target as well. There is no validation of the image,
and especially links are not checked in any way.
 
