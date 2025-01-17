require "fiddle/import"

module DX2LIB
  extend Fiddle::Importer
  dlload './dx2lib.so.2.8'
  extern 'void * DX2_OpenPort( const char *, long )'
  extern 'char DX2_Active( void * )'
  extern 'char DX2_Ping( void *, unsigned char, unsigned short * )'
  extern 'char DX2_ClosePort( void * )'
end

comport = "/dev/ttyUSB0"
br = 4000000
terr = ' ' * 2
mark = "\n"
devid = DX2LIB.DX2_OpenPort( comport, br )
if ( Fiddle::Pointer[devid].to_i != 0 ) then
  ret = DX2LIB.DX2_Active( devid )
  if ( ret == 1 ) then
    sleep 1
    i = 0
    while ( i <= 253 )
      ret = DX2LIB.DX2_Ping( devid, i, terr )
      if ( ret == 1 ) then
        printf( "\n%d %d alive", i , ret )
        mark = "\n"
      else
        printf( "%s%d %d dead(%x)", mark, i, ret, terr.unpack('S')[0] )
        mark = "\r"
      end
      i += 1
    end
    ret = DX2LIB.DX2_ClosePort( devid )
    if ( ret == 0 ) then
      print "CLOSE ERROR\n"
    end
  end
end
print("\r\nfin\r\n")
