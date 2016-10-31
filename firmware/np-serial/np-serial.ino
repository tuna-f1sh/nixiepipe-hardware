#include <FastLED.h>
#include <NixiePipe.h>

#define LED_PIN               6
#define NUM_PIPES             6
#define NUM_UNITS             0
#define BRIGHTNESS            255

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
#define VERSION_MAJOR         0

#define SERIAL_UINT16(LSB,MSB) (( (short) LSB & 0xFF) | (( (short) MSB & 0xFF) << 8))
#define SERIAL_UINT32(B0,B1,B2,B3) ( (long) B0 | ( (long) B1 << 8 ) | ( (long) B2 << 16 ) | ( (long) B3 << 24 ) )

NixiePipe pipes = NixiePipe(NUM_PIPES,NUM_UNITS);

typedef union {
  struct {
    byte size;
    byte command;
    byte message[MAX_MESSAGE];
  } data;
  byte bytes[MAX_PACKET];
} Packet_t;

bool gConnected = false;

static Packet_t processInput(void) {
  Packet_t packet;
  unsigned long timeout = millis();
  byte i = 0;

  packet.data.size = Serial.read();
  delay(1);
  packet.data.command = Serial.read();

  if (packet.data.size < MAX_MESSAGE) {
    while ( (i < (packet.data.size)) && ( (millis() - timeout) < 10) ) {
      packet.data.message[i++] = Serial.read();
      timeout = millis();
    }
  }

  return packet;
}

static void getColourPacket(byte *pmessage, CRGB *prgb) {
  prgb->r = pmessage[0];
  prgb->g = pmessage[1];
  prgb->b = pmessage[2];
}

static void processPacket(Packet_t *ppack) {
  CRGB rgb;
  byte pipe = ppack->data.message[0];
  Packet_t res;

  switch (ppack->data.command) {
    case (NP_SET_CONNECT):
      if (ppack->data.size >= 2) {
        if ((ppack->data.message[0] == 0x4E /*N*/) && (ppack->data.message[1] == 0x50 /*P*/)) {
          res.data.command = ppack->data.command;
          res.data.size = 2;
          res.data.message[0] = VERSION_MINOR;
          res.data.message[1] = VERSION_MAJOR;
          Serial.write(res.bytes,res.data.size+2);
          gConnected = true;
        }
      }
      break;
    case (NP_SET_NUMBER):
      if (ppack->data.size >= 4) {
        pipes.setNumber((long) SERIAL_UINT32(ppack->data.message[0],ppack->data.message[1],ppack->data.message[2],ppack->data.message[3]));
      }
      break;
    case (NP_SET_PIPE_NUMBER):
      if (ppack->data.size >= 2) {
        pipes.setPipeNumber(pipe,ppack->data.message[1]);
      }
      break;
    case (NP_SET_COLOUR):
      if (ppack->data.size >= 3) {
        getColourPacket(&ppack->data.message[0],&rgb);
        pipes.setPipeColour(rgb);
      }
      break;
    case (NP_SET_PIPE_COLOUR):
      if (ppack->data.size >= 4) {
        getColourPacket(&ppack->data.message[1],&rgb);
        pipes.setPipeColour(pipe,rgb);
      }
      break;
    case (NP_SET_BRIGHTNESS):
      if (ppack->data.size >= 1) {
        pipes.setBrightness(ppack->data.message[0]);
      }
      break;
    case (NP_SET_CLEAR):
      if (ppack->data.message[0]) {
        pipes.clear();
      }
      break;
    case (NP_SET_CLEAR_PIPE):
      if (ppack->data.size >= 1) {
        pipes.clearPipe(pipe);
      }
      break;
    case (NP_GET_NUMBER):
      break;
    case (NP_SET_UNITS):
      if (ppack->data.size >= 1) {
        pipes.setNumberUnits(ppack->data.message[0]);
      }
      break;
    case (NP_SET_SHOW):
      if (ppack->data.message[0]) {
        pipes.write();
        pipes.show();
      }
    default:
      pipes.writeSolid(CRGB::Red);
      pipes.show();
      Serial.write(0x3F);
      break;
  }
}

void setup()
{
  Serial.begin(57600);
  while (!Serial);

  pipes.begin();
  pipes.setPipeNumber(0,VERSION_MINOR);
  pipes.setPipeNumber(1,VERSION_MAJOR);
  pipes.write();
  pipes.show();
}

void loop()
{
  Packet_t packet;

  if (Serial.available()) {
    packet = processInput();
    processPacket(&packet);
  }

}
