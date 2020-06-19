# gala

An emulator of many incarnations of the Apple family of computers:
the ][, ][+, //e and //e enhanced.


## Features

* Slot 6: 140k disk images in .do, .po, .dsk, .nib and .2mg formats
* Multiple video modes
* Accepts custom configuration files for machine and media*
* Command line controls for essential settings and media insertion
* Slot 3: 80-column card emulation
* Slot 7: hard drive files in .hdv format
* One or two remappable joysticks
* Keyboard as joystick
* Mouse


## Build Requirements

*gala* runs on Debian/Ubuntu and Raspbian.
It could work on other Linux flavours.

Requires development libraries for SDL 1.2.


## Lineage

### AppleWin

-2015: Basis for initial port.

### LinApple

2015: Krez Beotiger refit AppleWin to use SDL and operate under Linux.

### linapple-pie

2017: Enhancements for Raspberry Pi were done by Mark Ormond:

  * mounting and running disks from the command line
  * joystick remapping

### Gala

2019-2020: Additional work on top to add more user-facing controls,
restore certain functionality, and improve RetroPie integration.


## References

https://github.com/linappleii
https://github.com/AppleWin
