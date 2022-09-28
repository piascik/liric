# Nudgematic library

This library is used to control 'nudgematic' mechanism on Raptor. This mechanism allows for the offseting of the pointing centre on the detector, without moving the telescope (which is slow and takes time to settle). It consists of two platform mounted on rails (horizontal and vertical). Both platforms can be moved by turning a motor attached to a cam, with odd-sized lobes giving two different sizes of offset in each direction. There is a series of microswitches on each platform, that are triggered by a series of configurable 'humps', so a set number of positions in each direction can be determined. The motors and microswitches are controlled / read by a USB-PIO board (and via the usb_pio library).

## Directory structure

* **c** The source code for the library.
* **include** The header files/API of the library.
* **test** The command line test programs for the library.

