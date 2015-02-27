zedboard-cheetah
================

Utilities for transferring (large) files into the ZedBoard over ethernet from a host for bare-bones projects.

Contains two sub-projects:

zedboard-datasend, a Qt application for sending data from the host side. Supports a tiny subset of xmd commands for executing tcl scripts, useful for batch processing.
zedboard-datarecv, example bare-bones application for receiving the data on the ZedBoard target and writing it into the DDR.

Status: copying data into ZedBoard works

TODO:
 - add support for reading from ZedBoard
 - add support for generating md5 hashes of data for quick verification
 - code cleanup and structuring
 - make zedboard side modular for easy integration into projects
