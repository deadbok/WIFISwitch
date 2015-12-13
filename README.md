WIFI controlled mains switch.
=============================

WIFI controlled mains switch using ESP8266.

Features.
---------

* Built in web server.
* RESTful (I think) interface.
* Enters AP mode and serves a WIFI configuration page, when there is no AP connection at start up.
* Captive portal in configuration mode.
* Multifunctional one button interface. :-D

Warning.
--------

**The communication is not encrypted!** Without insecurities this would not be a real IoT device,
though spying has yet to be implemented. Seriously though, **do not put this thing on the internet
in its current state.**

Credits.
========

### Peter Scargill (http://tech.scargill.net/) ###

Last piece in the puzzle, that turns off the ESP8266 messages, namely ets_printf
still works.

### Dave Hylands (http://www.davehylands.com/) ###

Suggested using a zip file, as file system image, and shared his code

### Todd Motto (http://toddmotto.com/) ###

Has a nice tutorial on AJAX and JavaScript.


