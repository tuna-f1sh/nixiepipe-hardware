#include "NixiePipe.h"

#if FASTLED_VERSION < 3000000
  #error "NixiePipe requires FastLED 3.0 lib: https://github.com/FastLED/FastLED"
#endif

inline uint8_t num2idx(uint8_t num) {
  return (num > 0) ? (num - 1) : 0;
}
static inline long powint(int factor, unsigned int exponent)
{
  long product = 1;
  while (exponent--)
    product *= factor;
  return product;
}

NixiePipe::NixiePipe(uint8_t n, uint8_t p) : numPipes(n), pin(p), numLEDs(0), pipeNum(0), brightness(0), modNum(0), maxNum(0), pixels(0), pipeColour(0)
{
  numLEDs = PIXEL_OFFSET * numPipes;
  maxNum = powint(10,numPipes) - 1;

  pixels = new CRGB[numLEDs]();
  pipeNum = new uint8_t[numPipes]{0};
  pipeColour = new CRGB[numPipes]{CRGB::White};
  
  FastLED.addLeds<LED_TYPE, 6, COLOR_ORDER>(pixels, numLEDs);
}

NixiePipe::~NixiePipe() {
  if(pixels) delete[] pixels;
  if(pipeNum) delete[] pipeNum;
  if(pipeColour) delete[] pipeColour;
}

void NixiePipe::passSerial(HardwareSerial &serial) {
  pserial = &serial;
}

uint32_t NixiePipe::getNumber(void) {
  return this->modNum;
}

void NixiePipe::setBrightness(uint8_t nvalue) {
  brightness = nvalue;
  FastLED.setBrightness(brightness);
}

void NixiePipe::setPipeNumber(uint8_t n, uint8_t num) {
  int16_t newd;
  newd = num - pipeNum[n];
  newd *= powint(10,n);
  // pserial->print("setPipeNumber: ");
  // pserial->println(num);
  // pserial->print("PipeNum: ");
  // pserial->println(pipeNum[n]);
  // pserial->print("Newd: ");
  // pserial->println(newd);

  pipeNum[n] = num; // update number reference
  this->modNum += newd; // update module reference number
  // pserial->print("modNum: ");
  // pserial->println(modNum);
}

void NixiePipe::setNumber(uint32_t num) {
  uint8_t digit = 0;
  uint8_t i = 0;
  // pserial->print("setNumber: ");
  // pserial->println(num);

  do {
    digit = num % 10;
    num /= 10;
    this->setPipeNumber(i++,digit);
  } while (i < this->numPipes);

  // write leading zeros
  // while (i < this->numPipes) {
  //   this->setPipeNumber(i++,digit);
  // }

}

void NixiePipe::writePipeNumber(uint8_t n, uint8_t num, CRGB c) {
  this->setPipeColour(n,c);

  this->setPipeNumber(n,num);
  this->write();
}

void NixiePipe::writePipeNumber(uint8_t n, uint8_t num) {
  this->writePipeNumber(n,num,this->pipeColour[n]);
}

void NixiePipe::setPipeColour(uint8_t n, CRGB c) {
  pipeColour[n] = c;
}

void NixiePipe::setPipeColour(CRGB c) {
  for (int i=0; i < numPipes; i++)
    this->setPipeColour(i,c);
}

void NixiePipe::writeNumber(uint8_t num, CRGB c) {
  this->setNumber(num);
  this->writeSolid(c);
}

void NixiePipe::writeNumber(uint8_t num) {
  this->setNumber(num);
  this->write();
}

void NixiePipe::show(void) {
  FastLED.show();
}

void NixiePipe::write(void) {
  this->clear();
  
  for (int i=0; i < numPipes; i++) {
    CRGB *ppipe = &pixels[i * PIXEL_OFFSET];
    if (pipeNum[i] > 0)
      ppipe[NUM2IDX(pipeNum[i])] = pipeColour[i];
    else
      this->writePipeFill(i,pipeColour[i]);
  }
}

void NixiePipe::writeSolid(CRGB c) {
  this->setPipeColour(c);
  this->write();
}

void NixiePipe::writeFade(uint8_t step) {
  for (int i = 0;i < numPipes; i++) {
    CRGB *ppipe = &pixels[i * PIXEL_OFFSET];
    fadeToBlackBy(ppipe,9,step); // turn off all pixels in block
    if (pipeNum[i] > 0)
      ppipe[NUM2IDX(pipeNum[i])] = pipeColour[i];
    else
      this->writePipeFill(i,pipeColour[i]);
    fadeLightBy(&ppipe[pipeNum[i]-1],1,step);
  }
}

void NixiePipe::writeRainbow(uint8_t gHue) {
  CHSV hsv;
  CRGB rgb;
  hsv.hue = gHue;
  hsv.val = 255;
  hsv.sat = 240;
  hsv2rgb_rainbow(hsv,rgb);

  for (int i=0; i < numPipes; i++) {
    CRGB *ppipe = &pixels[i * PIXEL_OFFSET];
    if (pipeNum[i] > 0)
      ppipe[NUM2IDX(pipeNum[i])] = hsv;
    else
      this->writePipeFill(i,rgb);
  }
}

void NixiePipe::writePipeFill(uint8_t n, CRGB c) {
  CRGBSet module = this->getPipe(n);

  module.fill_solid(c);
}


void NixiePipe::clear(void) {
  CRGBSet pipes(pixels,numLEDs);

  pipes.fill_solid(CRGB::Black);
}

void NixiePipe::clearPipe(uint8_t n) {
  CRGBSet module = this->getPipe(n);

  module.fill_solid(CRGB::Black);
}

CRGB* NixiePipe::getPixels() {
  return pixels;
}

CRGBSet NixiePipe::getPipe(uint8_t n) {
  CRGB *ppipe = &pixels[n * PIXEL_OFFSET];
  CRGBSet module(ppipe,PIXEL_OFFSET);
  return module;
}

NixiePipe& NixiePipe::operator++() {
  uint32_t temp = this->modNum;
  if (temp < this->maxNum )
    temp++;
  else
    temp = 0;
  this->setNumber(temp);
  return *this;
}

NixiePipe NixiePipe::operator++(int) {
  NixiePipe tmp = *this;
  ++*this;
  return tmp;
}

NixiePipe& NixiePipe::operator--() {
  uint32_t temp = this->modNum;
  if (temp > 0)
    temp--;
  else
    temp = this->maxNum;
  this->setNumber(temp);
  return *this;
}

NixiePipe NixiePipe::operator--(int) {
  NixiePipe tmp = *this;
  --*this;
  return tmp;
}

NixiePipe& NixiePipe::shift(int8_t n) {
  // NixiePipe tmp = *this;
  int idx = 0;

  if (n >= 0) {
    for(int i = (this->numPipes - 1); i >= 0; i--) {
      idx = i - n;
      this->pipeNum[i] = (idx >= 0 && idx < this->numPipes) ? this->pipeNum[idx] : 0;
    }
  } else {
    for(int i = 0; i < this->numPipes; i++) {
      idx = i - n;
      this->pipeNum[i] = (idx >= 0 && idx < this->numPipes) ? this->pipeNum[idx] : 0;
    }
  }

  
  return *this;
}

// NixiePipe& operator>>(const NixiePipe &rhs) {
//   ;;
// }

// NixiePipe& operator<<(const NixiePipe &rhs) {
//   ;;
// }
