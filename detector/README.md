A library to control the Raptor Ninox-640 infrared detector.
This uses a predefined set of fixed exposure length configuration files called 'format (fmt) files'.

We implement LT MULTRUNs as follows:
- As per IO:I/SupIRCam, use MULTRUN, we use 'No of exposures' to mean 'number of dithers'.
- For the MULTRUN exposure length, we divide by the selected fmt file exposure length, acquire that number of frames and then median stack them, to produce the resultant FITS image.

