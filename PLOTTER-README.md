## This is a short FAQ about the plotting library used in this project for testing purposes

### How can I install and use the library?
The library can be installed from the Library Manager under the name 'Plotter' (with a capital P). The GitHub repo for the library hosts [documentation](https://github.com/devinaconley/arduino-plotter/wiki/Installation-and-Quickstart). To use a program using the library, launch the listener app while the Arduino is running.

### How can I install the listener?
This depends on what operating system you are using. Refer to the list below.
- [Windows 32-bit](https://www.dropbox.com/s/vbgd4osedrfgep3/arduino-plotter-listener-windows32.zip?dl=1)
- [Windows 64-bit](https://www.dropbox.com/s/0x4xqv375h4j5hh/arduino-plotter-listener-windows64.zip?dl=1)
- [Linux 32-bit](https://www.dropbox.com/s/u0g1ey96aw14k9z/arduino-plotter-listener-linux32.zip?dl=1)
- [Linux 64-bit](https://www.dropbox.com/s/lsrpo5edb25sv3a/arduino-plotter-listener-linux64.zip?dl=1)
- [Mac OS](https://www.dropbox.com/s/k6ej5jftofotlhk/arduino-plotter-listener-macosx.zip?dl=1) [^1]

### Why is my code not working when I use Serial.print/println()?
The library needs exclusive access to the serial port so you cannot use any Serial.print()s etc. in your program.

[^1]: On Mac OS, there is an issue with Gatekeeper. Because of this, once you have extracted the app to the Applications folder, you have to open Terminal and run the command `xattr -rc /Applications/listener.app`.
