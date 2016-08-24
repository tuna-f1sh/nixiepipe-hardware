#include <Wire.h>
#include <TimeLib.h>
#include <FastLED.h>
#include <DS3232RTC.h>

#include <NixiePipe.h>

#define LED_PIN       6
#define NUM_PIPES     4
#define BRIGHTNESS    10

#define COLOUR        CRGB::White
#define RAINBOW       true // display rainbow digits

uint8_t gHue = 0;

NixiePipe pipes = NixiePipe(NUM_PIPES,LED_PIN);

static inline void setTime(tmElements_t tm) {
  pipes.setNumber((tm.Hour * 100) + tm.Minute);
}

void setup() {
  Serial.begin(9600);
  pipes.passSerial(Serial);

  pipes.clear();
  pipes.setBrightness(BRIGHTNESS);
  pipes.setPipeColour(COLOUR);
  pipes.writeNumber(0);
  pipes.show();
}

void loop() {
  tmElements_t tm; // time struct holder

  EVERY_N_SECONDS(1) {
    if (RTC.read(tm) == 0) {
      Serial.print(tm.Hour);
      Serial.print(":");
      Serial.println(tm.Minute);
      setTime(tm);
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
  
  EVERY_N_MILLISECONDS( 50 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  pipes.writeFade(4);
  if (RAINBOW) 
    pipes.writeRainbow(gHue);

  pipes.show();
}
