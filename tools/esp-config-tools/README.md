ESP8266 config tools.
=====================

Tools for handling configuration stored in the ESP8266 flash, as an
image before flashing.

Tools.
------

### ``gen_config`` ###

Create a binary configuration  image from command line options.

*This tool is firmware specific, it only works for the firmware it
was compiled as part of.*

Usage: ``gen_config`` [options] image_file fs_addr
Create configuration image, image_file, writing the following options.

Options:
 -v: Be verbose.

Configuration parameters:
 Variable "fs_addr": Address in flash of the file system.

