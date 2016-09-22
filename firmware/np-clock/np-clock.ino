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
#define ST_TERM       10

#define EV_ANY        -1
#define EV_NONE       0
#define EV_TB0_PRESS  10
#define EV_TB1_PRESS  11
#define EV_TB0_HOLD   12
#define EV_TB1_HOLD   13
#define EV_COUNTEND   14

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
bool alarmFlag = false;

int8_t gState = ST_CLOCK;
int8_t gEvent = EV_NONE;

NixiePipe pipes = NixiePipe(NUM_PIPES);

static int8_t processTB0(void) {
  uint32_t hold = millis();
  int8_t event = EV_NONE;

  while (!digitalRead(PIPE_TB0)) {
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

  while (!digitalRead(PIPE_TB1)) {
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

static void changeTime(tmElements_t *tm, int8_t dir) {
  for (int i = 0; i < abs(dir); ++i) {
    // UP
    if (dir >= 0) {
      if (tm->Minute == 59) {
        tm->Minute = 0;
        if (tm->Hour == 23) {
          tm->Hour = 0;
        } else {
          ++(tm->Hour);
        }
      } else {
        ++(tm->Minute);
      }
    // DOWN
    } else {
      if (tm->Minute == 0) {
        tm->Minute = 59;
        if (tm->Hour == 0) {
          tm->Hour = 23;
        } else {
          --(tm->Hour);
        }
      } else {
        --(tm->Minute);
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

  // save to RTC, clear ISR flag and resume interrupts
  RTC.write(tm);
  interrupts();

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

  // save to RTC, clear ISR flag and resume interrupts
  RTC.setAlarm(ALM1_MATCH_HOURS, 0, alarm.Minute, alarm.Hour, 0);
  interrupts();

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

  interrupts();

  return ST_CLOCK;
}

static int8_t setCounter(void) {
  uint32_t entry = millis(); // entry time - reset each button press
  uint32_t hold = millis(); // hold timer to increase time change
  uint32_t debounce = 0; // debouce holder
  uint8_t dperiod = DEBOUNCE; // debouce period

  // blank the counter if it isn't running
  if (gState != ST_CUPDATE)
    pipes.setNumber(0);

  // run for 5s then save the time to RTC
  while (millis() - entry < 5000) {

    // Set pipes red to show setting clock
    pipes.writeSolid(CRGB::Green);
    
    if (!digitalRead(PIPE_TB0)) {
      if (millis() - hold > 1000) {
        dperiod = 60;
      } else {
        dperiod = DEBOUNCE;
      }

      if ((millis() - debounce) > dperiod ) {
        entry = millis();
        ++pipes;
        debounce = entry;
      }
    } else if (!digitalRead(PIPE_TB1)) {
      if ((millis() - entry) > 1000 ) {
        entry = millis() + 6000;
        delay(500);
      }
    } else {
      hold = millis(); // reset the hold button as hasn't been pressed
    }

    pipes.show();
  }

  pipes.setPipeColour(gMainRGB);
  interrupts();

  return ST_COUNTER;
}

void setup() {
  Serial.begin(9600);
  pipes.passSerial(Serial);

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

static int8_t getEvent(void) {
  static unsigned long tb0db = 0;
  unsigned long tb0int = millis();
  static unsigned long tb1db = 0;
  unsigned long tb1int = millis();
  int8_t event = EV_NONE;

  if (!digitalRead(PIPE_TB0) & (tb0int - tb0db > DEBOUNCE)) {
    event = processTB0();
    tb0db = millis();
  } else if (!digitalRead(PIPE_TB1) & (tb1int - tb1db > DEBOUNCE)) {
    event = processTB1();
    tb1db = millis();
  }

  if ((gState == ST_COUNTER) && (pipes.getNumber() == 0)) {
    event = EV_COUNTEND;
  }

  return event;
}


void loop() {
  static tmElements_t tm; // time struct holder
  static CEveryNSeconds everySecond(1);

  gEvent = getEvent();
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
        if (RTC.read(tm) == 0) {
          Serial.print(tm.Hour);
          Serial.print(":");
          Serial.println(tm.Minute);
          pipes.setPipeColour(gMainRGB);
          // check alarm
          if (RTC.alarm(ALARM_1) || alarmFlag || RTC.alarm(ALARM_2)) {
            Serial.println("ALARM!");
            alarmFlag = true;
            // flash off every second
            if (tm.Second % 2 == 0)
              pipes.setPipeColour(CRGB::Black);
            // disable alarm flag after 1 minute
            if (tm.Second == 59)
              alarmFlag = false;
          }
          writeTime(tm);
        } else {
          // rtc isn't reading
          Serial.println("RTC read error!  Please check the circuitry.");
          Serial.println();
          pipes.setNumber(0);
          pipes.writeSolid(CRGB::Red); // all red
          pipes.show();
          return;
        }
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
  }
  
  EVERY_N_MILLISECONDS( 100 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  pipes.writeFade(4);
  /* pipes.writeRainbow(gHue);*/

  pipes.show();

}
