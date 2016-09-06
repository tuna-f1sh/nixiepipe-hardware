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

CRGB main_rgb = MAIN_RGB;
uint8_t gHue = 0;
bool tb0Flag = false;
bool tb1Flag = false;
bool setTimeFlag = false;
bool setColourFlag = false;
bool setAlarmFlag = false;
bool setCounterFlag = false;
bool alarmFlag = false;
bool counterFlag = false;

NixiePipe pipes = NixiePipe(NUM_PIPES,LED_PIN);

static void processTB0(void) {
  uint32_t hold = millis();

  while (!digitalRead(PIPE_TB0) && !setColourFlag) {
    if ((millis() - hold < 4000) && (millis() - hold > 200)) {
      counterFlag = false;
      setTimeFlag = true;
    } else if ((millis() - hold > 4000)) {
      setTimeFlag = false;
      setColourFlag = true;
    }
  }

  tb0Flag = false;
}

static void processTB1(void) {
  uint32_t hold = millis();

  while (!digitalRead(PIPE_TB1) && !setAlarmFlag) {
    if ((millis() - hold < 4000) && (millis() - hold > 200)) {
      setCounterFlag = true;
    } else if ((millis() - hold > 4000)) {
      setCounterFlag = false;
      counterFlag = false;
      setAlarmFlag = true;
    }
  }

  tb1Flag = false;
}

static inline void writeTime(tmElements_t tm) {
  pipes.setNumber((tm.Hour * 100) + tm.Minute);
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


static void setTime(tmElements_t &tm) {
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
  setTimeFlag = false;
  interrupts();

}

static void setAlarm(tmElements_t tm) {
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
  setAlarmFlag = false;
  interrupts();

}

static void setColour(void) {
  uint32_t entry = millis(); // entry time - reset each button press
  uint32_t debouce = 0;
  uint8_t hue = 0;
  uint8_t sat = 255;
  CHSV hsv(hue,sat,255);
  
  while(millis() - entry < 5000) {
    if (!digitalRead(PIPE_TB0)) {
      if ((millis() - debouce) > 10) {
        entry = millis();

        hsv2rgb_rainbow(hsv,main_rgb);
        pipes.writeSolid(main_rgb);
        pipes.show();
        hsv.hue = ++hue;

        debouce = millis();
      }
    } else if (!digitalRead(PIPE_TB1)) {
      if ((millis() - debouce) > 10) {
        entry = millis();

        hsv2rgb_rainbow(hsv,main_rgb);
        pipes.writeSolid(main_rgb);
        pipes.show();
        hsv.sat = --sat;

        debouce = millis();
      }
    }
  }

  setColourFlag = false;
  interrupts();
}

static void setCounter(void) {
  uint32_t entry = millis(); // entry time - reset each button press
  uint32_t hold = millis(); // hold timer to increase time change
  uint32_t debounce = 0; // debouce holder
  uint8_t dperiod = DEBOUNCE; // debouce period

  // blank the counter if it isn't running
  if (!counterFlag)
    pipes.setNumber(0);
  counterFlag = true;

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

  setCounterFlag = false;
  pipes.setPipeColour(main_rgb);
  interrupts();
}

void setup() {
  Serial.begin(9600);
  pipes.passSerial(Serial);

  pipes.clear();
  pipes.setBrightness(BRIGHTNESS);
  pipes.setPipeColour(MAIN_RGB);
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

void loop() {
  static tmElements_t tm; // time struct holder
  static unsigned long tb0db = 0;
  unsigned long tb0int = millis();
  static unsigned long tb1db = 0;
  unsigned long tb1int = millis();

  if (!digitalRead(PIPE_TB0) & (tb0int - tb0db > DEBOUNCE)) {
    tb0Flag = true;
    tb0db = tb0int;
  }

  if (!digitalRead(PIPE_TB1) & (tb1int - tb1db > DEBOUNCE)) {
    tb1Flag = true;
    tb1db = tb1int;
  }

  // Process interrupt button call
  if (tb0Flag) {
    processTB0();
  }

  if (setTimeFlag && !counterFlag) {
    setTime(tm);
  } else if (setColourFlag) {
    setColour();
  }

  if (tb1Flag) {
    processTB1();
  }

  if (setCounterFlag) {
    setCounter();
  } else if (setAlarmFlag) {
    setAlarm(tm);
  }

  EVERY_N_SECONDS(1) {
    if (counterFlag) {
      if (pipes.getNumber() > 0) {
        if (pipes.getNumber() <= 10)
          pipes.setPipeColour(CRGB::Red);
        --pipes;
      } else {
        counterFlag = false;
      }
    } else {
      if (RTC.read(tm) == 0) {
        Serial.print(tm.Hour);
        Serial.print(":");
        Serial.println(tm.Minute);
        pipes.setPipeColour(main_rgb);
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
  }
  
  
  EVERY_N_MILLISECONDS( 100 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  pipes.writeFade(4);
  /* pipes.writeRainbow(gHue);*/

  pipes.show();

}
