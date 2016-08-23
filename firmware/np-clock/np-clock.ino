#include <Wire.h>
#include <TimeLib.h>
#include <FastLED.h>
#include <DS3232RTC.h>

#include "NixiePipe.h"

#define LED_PIN       6
#define NUM_PIPES     1
#define BRIGHTNESS    255

uint8_t gHue = 0;

NixiePipe pipes = NixiePipe(NUM_PIPES,LED_PIN);

static inline void setTime(tmElements_t tm) {
  pipes.setNumber((tm.Hour * 100) + tm.Minute);
}

static void cycle(void) {
  for (int i = 1; i < 10; i++) {
    pipes.setPipeNumber(0,i);
    pipes.shift(1);
    pipes.setPipeNumber(0,i);
    pipes.write();
    pipes.show();
    delay(100);
  }
  for (int i = 9; i > 0; i--) {
    pipes.setPipeNumber(0,i);
    pipes.shift(1);
    pipes.setPipeNumber(0,i);
    pipes.write();
    pipes.show();
    delay(100);
  }
}  

void setup() {
  Serial.begin(9600);
  pipes.passSerial(Serial);

  pipes.clear();
  pipes.setBrightness(BRIGHTNESS);
  pipes.setPipeColour(CRGB::White);
  pipes.writePipeNumber(0,1);
  pipes.show();

  for (int i = 0; i < 10; i++) {
    pipes.setPipeNumber(0,i);
    pipes.shift(1);
    pipes.setPipeNumber(0,i);
    pipes.write();
    pipes.show();
    delay(100);
  }
  for (int i = 9; i > 0; i--) {
    pipes.setPipeNumber(0,i);
    pipes.shift(1);
    pipes.setPipeNumber(0,i);
    pipes.write();
    pipes.show();
    delay(100);
  }

  pipes.setNumber(0);
  pipes.clear();
  pipes.show();
}

void loop() {
  tmElements_t tm; // time struct holder
  static uint8_t i = 1;

  /* EVERY_N_SECONDS(1) {*/
  /*   if (RTC.read(tm) == 0) {*/
  /*     Serial.print(tm.Hour);*/
  /*     Serial.print(":");*/
  /*     Serial.println(tm.Minute);*/
  /*     setTime(tm);*/
  /*   } else {*/
  /*     // rtc isn't reading*/
  /*     Serial.println("RTC read error!  Please check the circuitry.");*/
  /*     Serial.println();*/
  /*     pipes.setNumber(0);*/
  /*     pipes.writeSolid(CRGB::Red); // all red*/
  /*     pipes.show();*/
  /*     return;*/
  /*   }*/
  /* }*/
  
  EVERY_N_SECONDS(1) { pipes.setPipeNumber(0,i);i = ((i < 9) ? (i + 1) : 1); }
  
  /* EVERY_N_MILLISECONDS(10) { ++pipes; }*/
  EVERY_N_MILLISECONDS( 100 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  pipes.writeFade(4);
  /* pipes.write();*/
  pipes.writeRainbow(gHue);

  pipes.show();

}
