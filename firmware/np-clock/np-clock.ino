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

#include <Wire.h>
#include <TimeLib.h>
#include <FastLED.h>
#include <DS3232RTC.h>

#include <NixiePipe.h>

#include "state_machine.h"
#include "serial.h"

#define LED_PIN       6
#define NUM_PIPES     4
#define BRIGHTNESS    255
#define MAIN_RGB      CRGB::OrangeRed
#define DEBOUNCE      200

#define RAINBOW       false

CRGB gMainRGB = MAIN_RGB;
uint8_t gHue = 0;

int8_t gState = ST_CLOCK;
int8_t gEvent = EV_NONE;
bool gConnected = false;
const char* message = "Hello Crux from Switzerland!";

NixiePipe pipes = NixiePipe(NUM_PIPES);

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
  static tmElements_t tm; // time struct holder to set time

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
    case (NP_SET_TIME):
      if (ppack->data.size >= 1) {
        packValue(&res, pipes.getNumber());
        tm.Hour = ppack->data.message[0];
        tm.Minute = ppack->data.message[1];
        RTC.write(tm);
      }
      break;
    default:
      /* pipes.writeSolid(CRGB::Red);*/
      /* pipes.show();*/
      /* res.data.message[0] = 0x3F;*/
      break;
  }

  Serial.write(res.bytes,res.data.size+2);
}

static int8_t processTB0(void) {
  uint32_t hold = millis();
  int8_t event = EV_NONE;

  while (!digitalRead(PIPE_TB0) && (event != EV_TB0_HOLD) ) {
    if ((millis() - hold < 4000) && (millis() - hold > 200)) {
      event = EV_TB0_PRESS;
    } else if ((millis() - hold > 4000)) {
      event = EV_TB0_HOLD;
    }
  }

  return event;
}

static int8_t processTB1(void) {
  uint32_t hold = millis();
  int8_t event = EV_NONE;

  while (!digitalRead(PIPE_TB1) && (event != EV_TB1_HOLD) ) {
    if ((millis() - hold < 4000) && (millis() - hold > 200)) {
      event = EV_TB1_PRESS;
    } else if ((millis() - hold > 4000)) {
      event = EV_TB1_HOLD;
    }
  }

  return event;
}

static inline void writeTime(tmElements_t tm) {
  uint32_t time_val;

  // show seconds with more than 6 pipes
  if (NUM_PIPES > 5) {
    time_val = tm.Hour;
    time_val *= 10000;
    time_val += tm.Minute * 100;
    time_val += tm.Second;
  // otherwise just hours and minutes
  } else {
    time_val = tm.Hour * 100;
    time_val += tm.Minute;
  }

  pipes.setNumber(time_val);
}

static void changeTime(tmElements_t *ptm, int8_t dir) {
  // set seconds to zero as we're not able to move them with buttons
  ptm->Second = 0;

  for (int i = 0; i < abs(dir); ++i) {
    // UP
    if (dir >= 0) {
      if (ptm->Minute == 59) {
        ptm->Minute = 0;
        if (ptm->Hour == 23) {
          ptm->Hour = 0;
        } else {
          ++(ptm->Hour);
        }
      } else {
        ++(ptm->Minute);
      }
    // DOWN
    } else {
      if (ptm->Minute == 0) {
        ptm->Minute = 59;
        if (ptm->Hour == 0) {
          ptm->Hour = 23;
        } else {
          --(ptm->Hour);
        }
      } else {
        --(ptm->Minute);
      }
    }
  }
}


static int8_t setTime(tmElements_t &tm) {
  uint32_t entry = millis(); // entry time - reset each button press
  uint32_t hold = millis(); // hold timer to increase time change
  uint32_t debounce = 0; // debouce holder
  uint8_t dperiod = DEBOUNCE; // debouce period

  writeTime(tm);
  pipes.show();

  // run for 5s then save the time to RTC
  while (millis() - entry < 5000) {

    // Set pipes red to show setting clock
    pipes.writeSolid(CRGB::Red);
    
    if (!digitalRead(PIPE_TB0)) {
      if (millis() - hold > 1000) {
        dperiod = 10;
      } else {
        dperiod = DEBOUNCE;
      }

      // increment time up
      if ((millis() - debounce) > dperiod ) {
        entry = millis();
        changeTime(&tm, 1);
        debounce = entry;
      }
    } else if (!digitalRead(PIPE_TB1)) {
      if (millis() - hold > 1000) {
        dperiod = 10;
      } else {
        dperiod = DEBOUNCE;
      }

      // increment time down
      if ((millis() - debounce) > dperiod ) {
        entry = millis();
        changeTime(&tm, -1);
        debounce = entry;
      }

    } else {
      hold = millis(); // reset the hold button as hasn't been pressed
    }

    writeTime(tm);
    pipes.show();
  }

  // save to RTC
  RTC.write(tm);

  return ST_CLOCK;

}

static int8_t setAlarm(tmElements_t tm) {
  uint32_t entry = millis(); // entry time - reset each button press
  uint32_t hold = millis(); // hold timer to increase time change
  uint32_t debounce = 0; // debouce holder
  uint8_t dperiod = DEBOUNCE; // debouce period
  tmElements_t alarm;

  // start from current time + 10min
  alarm = tm;
  changeTime(&alarm,10);
  writeTime(alarm);
  pipes.show();

  // run for 5s then save the time to RTC
  while (millis() - entry < 5000) {

    // Set pipes red to show setting clock
    pipes.writeSolid(CRGB::Orange);
    
    if (!digitalRead(PIPE_TB0)) {
      if (millis() - hold > 1000) {
        dperiod = 10;
      } else {
        dperiod = DEBOUNCE;
      }

      if ((millis() - debounce) > dperiod ) {
        entry = millis();
        changeTime(&alarm,1);
        debounce = entry;
      }
    } else if (!digitalRead(PIPE_TB1)) {
      if (millis() - hold > 1000) {
        dperiod = 10;
      } else {
        dperiod = DEBOUNCE;
      }

      if ((millis() - debounce) > dperiod ) {
        entry = millis();
        changeTime(&alarm, -1);
        debounce = entry;
      }

    } else {
      hold = millis(); // reset the hold button as hasn't been pressed
    }

    writeTime(alarm);
    pipes.show();
  }

  // save to RTC
  RTC.setAlarm(ALM1_MATCH_HOURS, 0, alarm.Minute, alarm.Hour, 0);

  return ST_CLOCK;

}

static int8_t setColour(void) {
  uint32_t entry = millis(); // entry time - reset each button press
  uint32_t debouce = 0;
  uint8_t hue = 0;
  uint8_t sat = 255;
  CHSV hsv(hue,sat,255);
  
  while(millis() - entry < 5000) {
    if (!digitalRead(PIPE_TB0)) {
      if ((millis() - debouce) > 10) {
        entry = millis();

        hsv2rgb_rainbow(hsv,gMainRGB);
        pipes.writeSolid(gMainRGB);
        pipes.show();
        hsv.hue = ++hue;

        debouce = millis();
      }
    } else if (!digitalRead(PIPE_TB1)) {
      if ((millis() - debouce) > 10) {
        entry = millis();

        hsv2rgb_rainbow(hsv,gMainRGB);
        pipes.writeSolid(gMainRGB);
        pipes.show();
        hsv.sat = --sat;

        debouce = millis();
      }
    }
  }

  return ST_CLOCK;
}

static int8_t setCounter(void) {
  uint32_t entry = millis(); // entry time - reset each button press
  uint32_t hold = millis(); // hold timer to increase time change
  uint32_t debounce = 0; // debouce holder
  uint16_t dperiod = DEBOUNCE; // debouce period
  Packet_t packet;

  // blank the counter if it isn't running
  if (gState != ST_CUPDATE)
    pipes.setNumber(1);

  // Set pipes red to show setting clock
  pipes.writeSolid(CRGB::Green);
  pipes.show();

  // run for 30s then escape
  while ( (millis() - entry) < 30000) {
    
    if (!digitalRead(PIPE_TB0)) {
      if (millis() - hold > 1000) {
        if ((pipes.getNumber() % 30) == 0)
          dperiod = 500;
        else
          dperiod = 60;
      } else {
        dperiod = DEBOUNCE;
      }

      if ((millis() - debounce) > dperiod ) {
        entry = millis();
        // jump to 1/2 minutes if we hit minutes whilst held down
        if ( ((pipes.getNumber() % 30) == 0) && (millis() - hold > 1000) ) {
          pipes.setNumber(pipes.getNumber() + 30);
        // otherwise increment
        } else {
          ++pipes;
        }
        debounce = entry;
      }

    // show the changes
    pipes.write();
    pipes.show();

    } else if (!digitalRead(PIPE_TB1)) {
      if ((millis() - entry) > 1000 ) {
        // escape while statement
        entry = millis() + 30001;
        // hold until exit to prevent re-trigger
        while(!digitalRead(PIPE_TB1));
      }
    } else {
      hold = millis(); // reset the hold button as hasn't been pressed
    }

    // check for serial request in this mode too and exit if we get connect packet
    if (Serial.available()) {
      packet = processInput();
      processPacket(&packet);
      return ST_CLOCK;
    }

  }

  pipes.setPipeColour(gMainRGB);
  pipes.show();

  return ST_COUNTER;
}

static inline void startUpDisplay() {
  uint16_t entry = millis();

  for (int p = 0; p < NUM_PIPES; p++) {
    for (int i = 0; i < 10; i++) {
      pipes.setPipeNumber(p,i);
      entry = millis();
      while( (millis() - entry ) < 60) {
        pipes.writeFade(32);
        pipes.show();
      }
    }
  }
  for (int p = 0; p < NUM_PIPES; p++) {
    for (int i = 9; i >= 0; i--) {
      pipes.setPipeNumber(p,i);
      entry = millis();
      while( (millis() - entry ) < 60) {
        pipes.writeFade(32);
        pipes.show();
      }
    }
  }

  pipes.writeNumber(0);
  pipes.clear();
  pipes.show();
}

static void writeAscii(const char* string, uint8_t len) {
  uint8_t c;

  for (c = 0; c < len; c++) {
    pipes.writeNumber((uint32_t) string[c]);
    pipes.show();
    delay(200);
  }
}

void setup() {
  Packet_t packet;
  uint16_t entry = millis();

  Serial.begin(BAUD);
  /* pipes.passSerial(Serial);*/

  pipes.begin<LED_PIN>();
  pipes.clear();
  pipes.setBrightness(BRIGHTNESS);
  pipes.setPipeColour(CRGB::White);
  pipes.writeNumber( (VERSION_MAJOR * 100) + VERSION_MINOR);
  pipes.show();

  pinMode(PIPE_TB0,INPUT); // TB0
  pinMode(PIPE_TB1,INPUT); // TB1

  // brief wait at boot for connection attempt as RTS resets on port open
  // show firmware version whilst we are at it
  while ( (millis() - entry) < 2500) {
    if (Serial.available()) {
      packet = processInput();
      processPacket(&packet);
    }
  }

  writeAscii(message, sizeof(message);

  pipes.setPipeColour(gMainRGB);
  if (!gConnected) {
    startUpDisplay();
  }
}

static int8_t getEvent(tmElements_t *ptm) {
  static unsigned long tb0db = 0;
  unsigned long tb0int = millis();
  static unsigned long tb1db = 0;
  unsigned long tb1int = millis();
  int8_t event = EV_NONE;

  static bool alarmFlag = false;
  static bool counterFlag = false;
  static uint8_t counterSeconds = 0;

  // Buttons clear flags and set event
  if (!digitalRead(PIPE_TB0) & (tb0int - tb0db > DEBOUNCE)) {
    event = processTB0();
    tb0db = millis();
    alarmFlag = false; counterFlag = false;
  } else if (!digitalRead(PIPE_TB1) & (tb1int - tb1db > DEBOUNCE)) {
    event = processTB1();
    tb1db = millis();
    alarmFlag = false; counterFlag = false;
  }

  // RTC chip not present or failing to operate
  if (RTC.read(*ptm) != 0) {
    event = EV_RTCFAIL;
  }

  // Counter has finished. Set flag for 30s to flash display for this period
  if ((gState == ST_COUNTER) && (pipes.getNumber() == 0)) {
    event = EV_COUNTEND;
    counterFlag = true;
    counterSeconds = ptm->Second; // set seconds holder to current seconds for relative 30s timer
  } else if (counterFlag) {
    if (ptm->Second % 2 == 0)
      event = EV_COUNTEND;
    // we've flashed for 30s, clear the flag
    if (ptm->Second == ( (counterSeconds < 30) ? (counterSeconds + 30) : (counterSeconds - 29) ) ) {
      counterFlag = false;
    }
  }

  // Alarm has sounded. Set flag for 60s to flash display for this period
  if (RTC.alarm(ALARM_1) || alarmFlag || RTC.alarm(ALARM_2)) {
    Serial.println("ALARM!");
    alarmFlag = true;
    // flash off every second
    if (ptm->Second % 2 == 0)
      event = EV_ALARM;
    // disable alarm flag after 1 minute
    if (ptm->Second == 59) {
      alarmFlag = false;
    }
  }

  return event;
}


void loop() {
  static tmElements_t tm; // time struct holder
  static CEveryNSeconds everySecond(1);

  // Clock mode unless in serial mode
  if (!gConnected) {
    gEvent = getEvent(&tm);
    for (uint8_t i = 0; i < TRANS_COUNT; i++) {
        if ((gState == trans[i].st) || (ST_ANY == trans[i].st)) {
            if ((gEvent == trans[i].ev) || (EV_ANY == trans[i].ev)) {
                gState = trans[i].nst;
                break;
            }
        }
    }

    switch (gState) {
      case ST_CLOCK:
        if (everySecond) {
          Serial.print(tm.Hour);
          Serial.print(":");
          Serial.println(tm.Minute);
          pipes.setPipeColour(gMainRGB);

          // flash off every second in alarm event
          /* if (gEvent == EV_ALARM)*/
          /*   pipes.setPipeColour(CRGB::Black);*/
          /* // flash every second whilst counter end*/
          /* if (gEvent == EV_COUNTEND)*/
          /*   pipes.setPipeColour(CRGB::Red);*/

          writeTime(tm);
        }
        break;
      case ST_SETTIME:
        setTime(tm);
        break;
      case ST_SETCOLOUR:
        setColour();
        break;
      case ST_SETALARM:
        setAlarm(tm);
        break;
      case (ST_SETCOUNT): case (ST_CUPDATE):
        setCounter();
        break;
      case ST_COUNTER:
        if (everySecond) {
          if (pipes.getNumber() <= 10)
            pipes.setPipeColour(CRGB::Red);
          --pipes;
        }
        break;
      case ST_RTCFAIL:
        // rtc isn't reading
        Serial.println("RTC read error!  Please check the circuitry.");
        Serial.println();
        pipes.setNumber(9999);
        pipes.writeSolid(CRGB::Red); // all red
        pipes.show();
        break;
      case ST_FLASH:
        if (everySecond) {
          if (gEvent == EV_ALARM)
            pipes.writeSolid(CRGB::Black);
          else if (gEvent == EV_COUNTEND)
            pipes.writeSolid(CRGB::Red);
          pipes.show();
        }
        break;
    }
    
    EVERY_N_MILLISECONDS( 100 ) { gHue++; } // slowly cycle the "base color" through the rainbow

    pipes.writeFade(4);

    // rainbow at midday and midnight in clock state
    if (gState == ST_CLOCK) {
      if ( ((tm.Hour == 0) || (tm.Hour == 12)) && (tm.Minute == 0) )
        pipes.writeRainbow(gHue);
      // always display rainbow if rainbow flag
      else if (RAINBOW)
        pipes.writeRainbow(gHue);
    }

    pipes.show();

  // in serial mode, just act on packets
  } else {
    Packet_t packet;
    if (Serial.available()) {
      packet = processInput();
      processPacket(&packet);
    }
  }

}
