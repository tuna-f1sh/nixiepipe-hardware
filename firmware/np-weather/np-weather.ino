/*
//  GPL 3.0 License

//  Copyright (c) 2016 John Whittington www.jbrengineering.co.uk

//  This program is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.

//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.

//     You should have received a copy of the GNU General Public License
//     along with this program.  If not, see <http:www.gnu.org/licenses/>.
*/

#include <FastLED.h>
#include <NixiePipe.h>

#include <DS3232RTC.h>

#define LED_PIN               6
#define NUM_PIPES             8
#define NUM_UNITS             0
#define BRIGHTNESS            255

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
#define NP_SET_WEATHER        0x51
#define NP_SET_TIME           0x52

#define MAX_PACKET            18
#define MAX_MESSAGE           MAX_PACKET - 2

#define VERSION_MINOR         0
#define VERSION_MAJOR         1

#define DEBOUNCE              300
#define DISPLAY_MODES         5
#define OUTSIDE_WEATHER       0
#define HUMIDITY              1
#define WIND_SPEED            2
#define INSIDE_TEMP           3
#define DISPLAY_TIME          4
#define DISPLAY_UPDATE        DISPLAY_MODES

#define SERIAL_UINT16(LSB,MSB) (( (short) LSB & 0xFF) | (( (short) MSB & 0xFF) << 8))
#define SERIAL_UINT32(B0,B1,B2,B3) ( (long) B0 | ( (long) B1 << 8 ) | ( (long) B2 << 16 ) | ( (long) B3 << 24 ) )

NixiePipe pipes = NixiePipe(NUM_PIPES,NUM_UNITS);

uint8_t gDisplayMode = 0;
CRGB gMainColour = CRGB::White;
tmElements_t gTm; // time struct holder
CRGBPalette16 gPal = CRGBPalette16( CRGB::Blue, CRGB::Cyan, CRGB::Yellow, CRGB::Red);

typedef union {
  struct {
    byte size;
    byte command;
    byte message[MAX_MESSAGE];
  } data;
  byte bytes[MAX_PACKET];
} Packet_t;

typedef struct {
  byte outside;
  byte symbol;
  byte wind;
  byte humidity;
  byte inside;
} weather_t;

weather_t gWeather = {
  .outside = 0,
  .symbol = 0,
  .wind = 0,
  .humidity = 0,
  .inside = 0,
};

bool gConnected = false;

static Packet_t processInput(void) {
  Packet_t packet;
  unsigned long timeout;
  byte i = 0;

  packet.data.size = Serial.read();
  delay(1);
  packet.data.command = Serial.read();

  timeout = millis();

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

static void packValue(Packet_t *pres, uint32_t value) {

  for (int i = 0; i < 4; ++i) {
    pres->data.message[i] = (byte) ((value >> (8 * i)) & 0xFF);
  }

  pres->data.size = 4;
}

static void processPacket(Packet_t *ppack) {
  CRGB rgb;
  byte pipe = ppack->data.message[0];
  Packet_t res;

  res.data.command = ppack->data.command;
  res.data.size = 1;
  res.data.message[0] = 0x00;

  switch (ppack->data.command) {
    case (NP_SET_CONNECT):
      if (ppack->data.size >= 2) {
        if ((ppack->data.message[0] == 0x4E /*N*/) && (ppack->data.message[1] == 0x50 /*P*/)) {
          res.data.command = ppack->data.command;
          res.data.size = 2;
          res.data.message[0] = VERSION_MINOR;
          res.data.message[1] = VERSION_MAJOR;
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
      break;
    case (NP_GET_NUMBER):
      if (ppack->data.message[0]) {
        packValue(&res, pipes.getNumber());
      }
      break;
    case (NP_SET_WEATHER):
      if (ppack->data.size >= 1) {
        packValue(&res, pipes.getNumber());
        gWeather.outside = ppack->data.message[0];
        gWeather.symbol = ppack->data.message[1];
        gWeather.wind = ppack->data.message[2] * 3.6; // convert from m/s to km/h
        gWeather.humidity = ppack->data.message[3];
        gWeather.inside = RTC.temperature() / 4;
      }
      break;
    case (NP_SET_TIME):
      if (ppack->data.size >= 1) {
        packValue(&res, pipes.getNumber());
        gTm.Hour = ppack->data.message[0];
        gTm.Minute = ppack->data.message[1];
        RTC.write(gTm);
      }
      break;
    default:
      pipes.writeSolid(CRGB::Red);
      pipes.show();
      res.data.message[0] = 0x3F;
      break;
  }

  Serial.write(res.bytes,res.data.size+2);
}

void setup()
{
  pinMode(PIPE_TB0,INPUT); // TB0
  pinMode(PIPE_TB1,INPUT); // TB1

  Serial.begin(BAUD);
  while (!Serial);

  pipes.begin();
  pipes.setNumberUnits(1);
  pipes.setPipeColour(CRGB::Magenta);
  pipes.setPipeNumber(0,VERSION_MINOR);
  pipes.setPipeNumber(2,VERSION_MAJOR);
  pipes.write();
  pipes.show();
  pipes.setPipeColour(gMainColour);
}

void loop()
{
  static unsigned long tb0db = 0;
  unsigned long tb0int = millis();
  static unsigned long tb1db = 0;
  unsigned long tb1int = millis();
  Packet_t packet;
  static uint8_t lastDisplay = gDisplayMode;
  static CEveryNSeconds everySecond(1);
  static CEveryNSeconds every60Second(60);
  uint8_t colourIndex;

  // RH
  if (~digitalRead(PIPE_TB0) & (tb0int - tb0db > DEBOUNCE)) {
    if (gDisplayMode < DISPLAY_MODES)
      ++gDisplayMode;
    tb0db = tb0int;
  }

  // LH
  if (~digitalRead(PIPE_TB1) & (tb1int - tb1db > DEBOUNCE)) {
    if (gDisplayMode > 0) 
      --gDisplayMode;
    tb1db = tb1int;
  }

  // If user buttons changed mode and a value has been set
  if ((lastDisplay != gDisplayMode)) {
    lastDisplay = gDisplayMode;
    switch (gDisplayMode) {
      case (OUTSIDE_WEATHER):
        colourIndex = map(gWeather.outside, -10, 40, 0, 255);
        pipes.setPipeColour(ColorFromPalette(gPal, colourIndex));
        /*pipes.setPipeColour(CRGB::White);*/
        pipes.setNumber(gWeather.outside);
        pipes.setPipeNumber(0,gWeather.symbol);
        pipes.setPipeColour(0,CRGB::OrangeRed);
        pipes.write();
        pipes.show();
        break;
      case (HUMIDITY):
        pipes.setPipeColour(CRGB::White);
        pipes.setNumber(gWeather.humidity);
        pipes.setPipeNumber(0,Weather::Percent);
        pipes.setPipeColour(0,CRGB::Cyan);
        pipes.write();
        pipes.show();
        break;
      case (WIND_SPEED):
        pipes.setPipeColour(CRGB::White);
        pipes.setNumber(gWeather.wind);
        pipes.setPipeNumber(0,Weather::Wind);
        pipes.setPipeColour(0,CRGB::White);
        pipes.write();
        pipes.show();
        break;
      case (INSIDE_TEMP):
        gWeather.inside = RTC.temperature() / 4;
        colourIndex = map(gWeather.inside, 0, 30, 0, 255);
        pipes.setPipeColour(ColorFromPalette(gPal, colourIndex));
        /*pipes.setPipeColour(CRGB::White);*/
        pipes.setNumber(gWeather.inside);
        pipes.write();
        pipes.clearPipe(0);
        pipes.show();
        break;
      case (DISPLAY_TIME):
        RTC.read(gTm);
        pipes.setPipeColour(CRGB::OrangeRed);
        pipes.setPipeColour(0, CRGB::Black);
        pipes.setNumber(gTm.Hour * 100 + gTm.Minute);
        pipes.write();
        /*pipes.writePipeFill(0,CRGB::OrangeRed);*/ // all on but gets hot
        pipes.show();
        break;
      default:
        break;
    }
  }

  // Check for new data
  if (Serial.available()) {
    packet = processInput();
    processPacket(&packet);
    lastDisplay = DISPLAY_UPDATE;
  }

  // Update internal temp
  if (every60Second) {
    if (gDisplayMode == INSIDE_TEMP) {
      lastDisplay = DISPLAY_UPDATE;
      gWeather.inside = RTC.temperature() / 4;
    }
  }

  /*for (int i = 0; i < 255; i++) {*/
    /*pipes.setPipeColour(ColorFromPalette(gPal, i));*/
    /*pipes.write();*/
    /*pipes.show();*/
  /*}*/

  // Update time
  if (everySecond) {
    if (gDisplayMode == DISPLAY_TIME) {
      lastDisplay = DISPLAY_UPDATE;
      if (RTC.read(gTm) != 0) {
        pipes.setPipeColour(CRGB::Red);
      }
    }
  }
}
