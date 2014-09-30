zedboard-cheetah
================

Utilities for transferring (large) files into the ZedBoard over ethernet from a host for bare-bones projects.

Contains two sub-projects:

zedboard-datasend, a Qt application for sending data from the host side.
zedboard-datarecv, example bare-bones application for receiving the data on the ZedBoard target and writing it into the DDR.

Status: not yet usable, target only prints received packet sizes (to verify that we actually received everything).


TODO:
 - decide on format for memory write command format
 - implement some handshaking
 - implement write commands on host side
 - implement write commands on target side
 - add read (from host) support

