[![License](https://img.shields.io/github/license/esrf-bliss/lima.svg?style=flat)](https://opensource.org/licenses/GPL-3.0)
[![Gitter](https://img.shields.io/gitter/room/esrf-bliss/lima.svg?style=flat)](https://gitter.im/esrf-bliss/LImA)

# LImA Andor SDK3 Camera Plugin

A fully spectroscopic hard X-ray imaging detector
The HEXITEC detector measures the energy and position of every photon in the 4-200keV range. Each one of the 80x80 pixels provides a full energy spectrum with an average energy resolution of 800eV FWHM at 60keV.
The GigE 8080 is a self-contained, CE marked module that only requires a mains power supply and connection to a PC or Laptop. It can be supplied with a user friendly GUI to operate the detector and provide calibrated spectra per pixel or industry standard Gig-E-Vision APIs for users to integration into their own systems.

# Install

There is no conda packages for this detector family. One should compile from the main project source code. Refer to the Lima documentation below.

Lima ( **L** ibrary for **Im** age **A** cquisition) is a project for the unified control of 2D detectors. The aim is to clearly separate hardware specific code from common software configuration and features, like setting standard acquisition parameters (exposure time, external trigger), file saving and image processing.

Lima is a C++ library which can be used with many different cameras. The library also comes with a [Python](http://python.org) binding and provides a [PyTango](http://pytango.readthedocs.io/en/stable/) device server for remote control.

## Documentation

The documentation is available [here](https://lima.blissgarden.org)


