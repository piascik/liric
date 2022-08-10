# Raptor

Source code for the Raptor instrument.

This is a near infra-red imaging instrument, based around a Raptor Photonics Ninox 640 camera, with a shortwave indium gallium arsenide (InGaAs) detector.

The instrument also has a starlight express filter wheel, and an IO board driving an offsetting mechanism.

## Directory Structure

* **fmt** Raptor configuration files
* **c** This contains the source code for the C layer, that is sent commands from the Java software and controls the mechanisms.
* **include** The header files for the C layer source code.
* **scripts** Deployment and engineering scripts.
* **java** This contains the source code for the robotic layer, which receives commands from the LT robotic control system.
* **detector** This is a C library that uses the Raptor SDK to provide a library to control the Raptor detector.
* **filter_wheel** Starlight Express filter wheel control library

The Makefile.common file is included in Makefile's to provide common root directory information.
