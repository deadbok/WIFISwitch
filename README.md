WIFI controlled mains switch.
=============================

Unclouded WIFI controlled mains switch using ESP8266.

Features.
---------

The motivation for this projects, has been, and is, to have a standalone
wireless mains switch. There are a lot of nice IOT protocols, but they
tend to have a central hub/server, which is good in some situations, but 
not what I wanted. 
There is nothing keeping you from centralising management, the wifiswitch
uses standard HTTP and WebSocket communication protocols.

* Unclouded operation.
* Built in web server.
* WebSocket communication.
* Enters AP mode and serves a WIFI configuration page, when there is no
  AP connection at start up.
* Captive portal in configuration mode.
* Multifunctional one button interface. :-D

Warning.
--------

**The communication is not encrypted!** Without insecurities this would
not be a real IoT device, though spying has yet to be implemented.
Seriously though, **do not put this thing on the internet in its current
state.**

Credits (In order of appearance).
=================================

#### Paul Sokolovsky (http://pfalcon-oe.blogspot.com/) ####

esp-open-sdk, the toolchain I use to build this.

#### Dave Hylands (http://www.davehylands.com/) ####

Suggested using a zip file, as file system image, and shared his code

#### Todd Motto (http://toddmotto.com/) ####

Has a nice tutorial on AJAX and JavaScript.

#### Peter Scargill (http://tech.scargill.net/) ####

Last piece in the puzzle, that turns off the ESP8266 messages, namely
ets_printf still works.

#### SuperHouse (http://superhouse.tv) ####

esp-open-rtos, open implementation of the FreeRTOS SDK, that I use as
the base framework.

#### Richard A. Burton (http://richard.burtons.org/) ####

Taught me to use critical section when writing to flash, by sharing his
rboot code.

