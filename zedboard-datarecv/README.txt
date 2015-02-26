Modified from Xilinx' lwip echo server example, with some changes:

 - enable DHCP (and print obtained address onto terminal) instead of static IP
 - implement a very simple protocol for reading/writing ZedBoard memory. Reading does not work properly, only writing. Check echo.c to see what it looks like.

 Use the lwip example already present in the SDK to get all the correct settings and libraries.
 Remember to enable lwip_dhcp in the BSP options.