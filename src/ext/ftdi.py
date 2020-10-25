#!/usr/bin/env python

import os
import sys
import ftdi1 as ftdi
import time

ftdic = ftdi.new()
if ftdic ==0:
	print( 'new failed: %d', ret )
	os._exit( 1 )

# try to list ftdi devices 0x6010 or 0x6001
ret, devlist = ftdi.usb_find_all( ftdic, 0x0403, 0x6010 )
if ret <= 0:
    ret, devlist = ftdi.usb_find_all( ftdic, 0x0403, 0x6001)

if ret < 0:
    print( 'ftdi_usb_find_all failed: %d (%s)' % ( ret, ftdi.get_error_string( ftdic ) ) )
    os._exit( 1 )
print( 'Number of FTDI devices found: %d\n' % ret )
curnode = devlist
i = 0
while( curnode != None ):
    ret, manufacturer, description, serial = ftdi.usb_get_strings( ftdic, curnode.dev )
    if ret < 0:
        print( 'ftdi_usb_get_strings failed: %d (%s)' % ( ret, ftdi.get_error_string( ftdic ) ) )
        os._exit( 1 )
    print( 'Device #%d: manufacturer="%s" description="%s" serial="%s"\n' % ( i, manufacturer, description, serial ) )
    curnode = curnode.next
    i += 1

# open usb
ret = ftdi.usb_open( ftdic, 0x0403, 0x6001 )
if ret < 0:
    print( 'unable to open ftdi device: %d (%s)' % ( ret, ftdi.get_error_string( ftdic ) ) )
    os._exit( 1 )


# bitbang
ret = ftdi.set_bitmode( ftdic, 0xff, ftdi.BITMODE_BITBANG )
if ret < 0:
    print( 'Cannot enable bitbang' )
    os._exit( 1 )

#for i in range( 8 ):
val = 0x0
print( 'enabling bit #%d (0x%02x)' % (i, val) )
ftdi.write_data( ftdic, chr(val) )
time.sleep ( 5.5 )

val = 0x2
print( 'enabling bit #%d (0x%02x)' % (i, val) )
ftdi.write_data( ftdic, chr(val) )
time.sleep ( 5.5 )

#ftdi.write_data( ftdic, chr(0x00))
time.sleep ( 0.2 )

ftdi.disable_bitbang( ftdic )
print( '' )



# close usb
ret = ftdi.usb_close( ftdic )
if ret < 0:
    print( 'unable to close ftdi device: %d (%s)' % ( ret, ftdi.get_error_string( ftdic ) ) )
    os._exit( 1 )

print ('device closed')
ftdi.free( ftdic )
