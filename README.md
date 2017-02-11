# usb_luminos_reader

This tool is used to read the value from the Cleware USB-Luminos sensor
available from the [cleware-shop](http://www.cleware-shop.de).

I built the tool using Linux, but as I use [HIDAPI](http://www.signal11.us/oss/hidapi/) to access
USB it should also work on Windows and Mac.

Unfortunately I found very little information about the interface of the sensor, so I had to reverse engineer it.

There is a much more complete libraray for the Cleware devices available for Windows directly from Cleware. For Linux and Mac there is [clewarecontrol](https://www.vanheusden.com/clewarecontrol/) but unfortunately this did not work for me.

# Building

CMake is used as a build system.
just run ```cmake . && make``` to build the tool.

For usage run ```usb_luminos_reader -h```
