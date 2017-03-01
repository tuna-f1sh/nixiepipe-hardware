#define BAUD                  57600
#define NP_SET_NUMBER         0x40
#define NP_SET_PIPE_NUMBER    0x41
#define NP_SET_COLOUR         0x42
#define NP_SET_PIPE_COLOUR    0x43
#define NP_SET_BRIGHTNESS     0x44
#define NP_SET_CLEAR          0x45
#define NP_SET_CLEAR_PIPE     0x46
#define NP_GET_NUMBER         0x47
#define NP_SET_CONNECT        0x48
#define NP_SET_UNITS          0x49
#define NP_SET_SHOW           0x50

#define MAX_PACKET            18
#define MAX_MESSAGE           MAX_PACKET - 2

#define VERSION_MINOR         1
#define VERSION_MAJOR         1

#define SERIAL_UINT16(LSB,MSB) (( (short) LSB & 0xFF) | (( (short) MSB & 0xFF) << 8))
#define SERIAL_UINT32(B0,B1,B2,B3) ( (long) B0 | ( (long) B1 << 8 ) | ( (long) B2 << 16 ) | ( (long) B3 << 24 ) )

typedef union {
  struct {
    byte size;
    byte command;
    byte message[MAX_MESSAGE];
  } data;
  byte bytes[MAX_PACKET];
} Packet_t;
