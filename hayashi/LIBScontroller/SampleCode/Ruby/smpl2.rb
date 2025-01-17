require "fiddle/import"

module DX2LIB
  extend Fiddle::Importer
  dlload './dx2lib.so.2.8'
  extern "void * DX2_OpenPort( const char *, unsigned long )"
  extern "char DX2_Active( void * )"
  extern "char DX2_Ping( void *, unsigned char, unsigned short * )"
  extern "char DX2_ReadLongData( void *, unsigned char, unsigned char, unsigned long *, unsigned short * )"
  extern "char DX2_WriteLongData( void *, unsigned char, unsigned char, unsigned long, unsigned short * )"
  extern "char DX2_ReadWordData( void *, unsigned char, unsigned char, unsigned short *, unsigned short * )"
  extern "char DX2_WriteWordData( void *, unsigned char, unsigned char, unsigned short, unsigned short * )"
  extern "char DX2_ReadByteData( void *, unsigned char, unsigned char, unsigned char *, unsigned short * )"
  extern "char DX2_WriteByteData( void *, unsigned char, unsigned char, unsigned char, unsigned short * )"
  extern "char DX2_ClosePort( long )"
end
comport = "/dev/ttyUSB0"
br = 57600
uid = 1
TORQUE_ENABLE_ADDR = 64
GOAL_POS_ADDR = 116
CURRENT_LIMIT_ADDR = 38
PRESENT_POS_ADDR = 132
MAX_POSITION = 4095
MIN_POSITION = 0
SLEEP_TIME = 0.05
LOOP_COUNT = 5
PARTIAL_COUNT = 20
OK = 1
NG = 0
terr = ' ' * 2
rbuff = ' ' * 4
pos = 0
devid = DX2LIB.DX2_OpenPort( comport, br )
if ( Fiddle::Pointer[devid].to_i != 0 ) then
  if ( DX2LIB.DX2_Ping( devid, uid, terr ) == OK ) then
    if ( DX2LIB.DX2_WriteWordData( devid, uid, CURRENT_LIMIT_ADDR, 300, terr ) != OK ) then
      printf( "current limit write error(%x)\n\r", terr.unpack('S')[0] )
    end
    DX2LIB.DX2_WriteByteData( devid, uid, TORQUE_ENABLE_ADDR, 1, terr )
    i = 0
    while( i < LOOP_COUNT )
      if ( pos == MAX_POSITION ) then
        pos = MIN_POSITION
      else
        pos = MAX_POSITION
      end
      if ( DX2LIB.DX2_WriteLongData( devid, uid, GOAL_POS_ADDR, pos, terr ) == OK ) then
        j = 0
        while( j < PARTIAL_COUNT )
          if ( DX2LIB.DX2_ReadLongData( devid, uid, PRESENT_POS_ADDR, rbuff, terr ) == OK ) then
            printf("POS:%d   \r", rbuff.unpack('s')[0])
          else
            printf("read error(%x)\n", terr.unpack('S')[0])
          end
          sleep SLEEP_TIME
          j += 1
        end
      else
        printf( "write error(%x)\n", terr.unpack('S')[0] )
      end
      i += 1
    end
    DX2LIB.DX2_WriteByteData( devid, uid, TORQUE_ENABLE_ADDR, 0, terr )
    if ( DX2LIB.DX2_ClosePort( devid ) != OK ) then
      print "CLOSE ERROR\n"
    end
  else
    p "Motor dead"
  end
else
  print "Port Open Failed"
end
print("\r\nfin\r\n")
