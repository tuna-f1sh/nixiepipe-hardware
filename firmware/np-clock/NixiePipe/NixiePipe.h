#ifndef NIXIEPIPE_H
#define NIXIEPIPE_H

#if (ARDUINO >= 100)
 #include <Arduino.h>
#else
 #include <WProgram.h>
 #include <pins_arduino.h>
#endif

#include <FastLED.h>

#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB
#define PIXEL_OFFSET        9 // pixel index offset per module

#define NUM2IDX(x)    x - 1

inline uint8_t num2idx(uint8_t num);

class NixiePipe {
  public: 

    // pass fading flag??
    NixiePipe(uint8_t n, uint8_t p);
    ~NixiePipe();

    void
      // begin(void),
      show(void), // FastLED show
      passSerial(HardwareSerial &serial),
      write(void),
      writeFade(uint8_t step), // fade to black, write lit LEDs call repeat
      setPipeNumber(uint8_t n, uint8_t num), // set number but don't clear previous
      setNumber(uint32_t num), // set number to array
      writePipeNumber(uint8_t n, uint8_t num), // set and clear previous (set black)
      writePipeNumber(uint8_t n, uint8_t num, CRGB c), // set and clear with colour
      setPipeColour(uint8_t n, CRGB c), // set pipe colour
      setPipeColour(CRGB c), // set pipe colour
      writeNumber(uint8_t num, CRGB C),
      writeNumber(uint8_t num),
      writeSolid(CRGB c), // set pipe colour
      writeRainbow(uint8_t gHue), // set pipe rainbow
      writePipeFill(uint8_t n, CRGB c),
      setBrightness(uint8_t brightness),
      clearPipe(uint8_t n),
      clear();
    CRGB
      *getPixels(void);
    CRGBSet
      getPipe(uint8_t n);
    NixiePipe
      &shift(int8_t n),
      &operator++(),
      &operator--(),
      operator++(int),
      operator--(int);
      // &operator>>(const NixiePipe &rhs),
      // &operator<<(const NixiePipe &rhs);
    uint32_t getNumber(void);

  private:
    const uint8_t
      numPipes;
    uint8_t
      pin,
      numLEDs,
      *pipeNum,
      brightness;
    uint32_t
      modNum,
      maxNum;
    CRGB 
      *pixels,
      *pipeColour;
    HardwareSerial *pserial;
};

#endif // NIXIEPIPE_H
