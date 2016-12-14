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

#define LED_PIN       6
#define NUM_PIPES     4
#define BRIGHTNESS    255
#define MAIN_RGB      CRGB::White
#define DEBOUNCE      200

#define ST_ANY        -1
#define ST_CLOCK      0
#define ST_SETTIME    1
#define ST_SETALARM   2
#define ST_SETCOLOUR  3
#define ST_SETCOUNT   4
#define ST_COUNTER    5
#define ST_CUPDATE    6
#define ST_RTCFAIL    7
#define ST_FLASH      8
#define ST_DIM        9
#define ST_TERM       10

#define EV_ANY        -1
#define EV_NONE       0
#define EV_TB0_PRESS  10
#define EV_TB1_PRESS  11
#define EV_TB0_HOLD   12
#define EV_TB1_HOLD   13
#define EV_COUNTEND   14
#define EV_RTCFAIL    15
#define EV_ALARM      16

typedef struct {
  int st;
  int ev;
  /* int (*fn)(void);*/
  int nst;
} tTransition;

tTransition trans[] = {
  { ST_CLOCK, EV_TB0_PRESS, ST_SETTIME },
  { ST_CLOCK, EV_TB0_HOLD, ST_SETCOLOUR },
  { ST_CLOCK, EV_TB1_PRESS, ST_SETCOUNT },
  { ST_CLOCK, EV_TB1_HOLD, ST_SETALARM },
  { ST_CLOCK, EV_RTCFAIL, ST_RTCFAIL },
  { ST_CLOCK, EV_ALARM, ST_FLASH },
  { ST_CLOCK, EV_COUNTEND, ST_FLASH },
  { ST_FLASH, EV_NONE, ST_CLOCK },
  { ST_FLASH, EV_TB0_PRESS, ST_CLOCK },
  { ST_FLASH, EV_TB0_HOLD, ST_CLOCK },
  { ST_FLASH, EV_TB1_PRESS, ST_CLOCK },
  { ST_FLASH, EV_TB1_HOLD, ST_CLOCK },
  { ST_RTCFAIL, EV_NONE, ST_CLOCK },
  { ST_SETTIME, EV_ANY, ST_CLOCK },
  { ST_SETALARM, EV_ANY, ST_CLOCK },
  { ST_SETCOUNT, EV_ANY, ST_COUNTER },
  { ST_SETCOLOUR, EV_ANY, ST_CLOCK },
  { ST_COUNTER, EV_COUNTEND, ST_CLOCK },
  { ST_COUNTER, EV_TB0_PRESS, ST_CLOCK },
  { ST_COUNTER, EV_TB0_HOLD, ST_CLOCK },
  { ST_COUNTER, EV_TB1_PRESS, ST_CUPDATE },
  { ST_COUNTER, EV_TB1_HOLD, ST_CLOCK },
  { ST_CUPDATE, EV_ANY, ST_COUNTER },
};

#define TRANS_COUNT (sizeof(trans)/sizeof(*trans))

CRGB gMainRGB = MAIN_RGB;
uint8_t gHue = 0;

int8_t gState = ST_CLOCK;
int8_t gEvent = EV_NONE;

NixiePipe pipes = NixiePipe(NUM_PIPES);

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
  if (NUM_PIPES <= 5)
    pipes.setNumber((tm.Hour * 100) + tm.Minute);
  else
    pipes.setNumber((tm.Hour * 10000) + (tm.Minute * 100) + tm.Second);
}

static void changeTime(tmElements_t *ptm, int8_t dir) {
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

  // blank the counter if it isn't running
  if (gState != ST_CUPDATE)
    pipes.setNumber(1);

  // run for 30s then escape
  while (millis() - entry < 30000) {

    // Set pipes red to show setting clock
    pipes.writeSolid(CRGB::Green);
    
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
    } else if (!digitalRead(PIPE_TB1)) {
      if ((millis() - entry) > 1000 ) {
        entry = millis() + 6000;
        // hold until exit to prevent re-trigger
        while(!digitalRead(PIPE_TB1));
      }
    } else {
      hold = millis(); // reset the hold button as hasn't been pressed
    }

    pipes.show();
  }

  pipes.setPipeColour(gMainRGB);

  return ST_COUNTER;
}

void setup() {
  Serial.begin(9600);
  /* pipes.passSerial(Serial);*/

  pipes.begin<LED_PIN>();
  pipes.clear();
  pipes.setBrightness(BRIGHTNESS);
  pipes.setPipeColour(gMainRGB);
  pipes.show();

  for (int i = 0; i < 10; i++) {
    pipes.shift(1);
    pipes.setPipeNumber(0,i);
    pipes.write();
    pipes.show();
    delay(100);
  }
  for (int i = 9; i > 0; i--) {
    pipes.shift(1);
    pipes.setPipeNumber(0,i);
    pipes.write();
    pipes.show();
    delay(100);
  }

  pinMode(PIPE_TB0,INPUT); // TB0
  pinMode(PIPE_TB1,INPUT); // TB1

  pipes.writeNumber(0);
  pipes.clear();
  pipes.show();
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

  // rainbow at midday and midnight
  if ( ((tm.Hour == 0) || (tm.Hour == 12)) && (tm.Minute == 0) && (gState != ST_RTCFAIL) )
    pipes.writeRainbow(gHue);
  else
    pipes.writeFade(4);


  pipes.show();

}
