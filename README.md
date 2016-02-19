# cartscan

Code for OPTICON [OPH-1005](http://www.opticonusa.com/products/handheld-solutions/oph-1005.html)
barcode scanner for tracking carts and a Windows application for
upload/download.

# Build the Windows application

Install MinGW and add it to the path:

	set path=c:\mingw\bin;$Path

Build with:

	mingw32-make

# Build the Opticon OPH-1005 application

Install SDK and be sure to include the OPH-1005 scanner and the Rx compiler. 
Replace the demo.c with the one here and type 'make'.

Turn off scanner.  Hold period, 1 and then hit power.  Select "Application
download".  Use the Appload application from Opticon to download the .hex
file.

