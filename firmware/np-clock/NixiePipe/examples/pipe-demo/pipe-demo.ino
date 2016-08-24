#include <FastLED.h>

#include <NixiePipe.h>

#define LED_PIN       6
#define NUM_PIPES     1
#define BRIGHTNESS    255
#define FADE_DEC	    20

#define FRAMES_PER_SECOND  128

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0;                  // Rainbow hue inc.
uint8_t i = 0;                     // Master number inc.
uint32_t pipesMax = 0;             // Max number pipes can show set by pipes.getMax()

void cycleUp();
void cycleDown();
void cycleBlock();
void shifter();
void rainbow();
void rainbowWithGlitter();
void confetti();
void sinelon();
void juggle();

NixiePipe pipes = NixiePipe(NUM_PIPES,LED_PIN);

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
#if (NUM_PIPES > 3)
  SimplePatternList gPatterns = { cycleUp, cycleDown, shifter, rainbow, rainbowWithGlitter, confetti, sinelon, juggle };
#else
  SimplePatternList gPatterns = { shifter, cycleBlock, rainbow, rainbowWithGlitter, confetti, sinelon, juggle };
#endif

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void setup() {
  pipes.clear();
  pipes.setBrightness(BRIGHTNESS);
  pipes.show();
  pipesMax = pipes.getMax();
}

void loop() {
    // Call the current pattern function once, updating the 'leds' array
	gPatterns[gCurrentPatternNumber]();

	pipes.show();

	FastLED.delay(1000/FRAMES_PER_SECOND);

	// do some periodic updates
	EVERY_N_MILLISECONDS(500) { (i < pipesMax) ? ++i : i=0; }
	EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
	EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
}

void nextPattern()
{
	// add one to the current pattern number, and wrap around at the end
	gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void cycleUp() {
	//pipes.clear();
	++pipes;
	pipes.write();
}

void cycleDown() {
	//pipes.clear();
	--pipes;
	pipes.write();
}

void cycleBlock() {
  for (int p = 0; p < NUM_PIPES; p++)
    pipes.setPipeNumber(p, i % 10);
    pipes.writeFade(FADE_DEC);
}

void shifter() {
	pipes.shift(1);
	pipes.setPipeNumber(0,i % 10);
	pipes.write();
}

void rainbow()
{
	cycleBlock();
	pipes.writeRainbow(gHue);
}

void rainbowWithGlitter()
{
	addGlitter(8);
	rainbow();
}

void addGlitter( fract8 chanceOfGlitter)
{
	CRGB* leds = pipes.getPixels();
	if( random8() < chanceOfGlitter) {
		leds[ random16(NUM_PIPES*9) ] += CRGB::White;
	}
}

void confetti()
{
	CRGB rgb;

	pipes.setNumber(i);
	hsv2rgb_rainbow(CHSV( gHue + random8(64), 200, 255),rgb);
  pipes.setPipeColour(rgb);
	pipes.writeFade(FADE_DEC);
}

void sinelon()
{
	CRGB rgb;

	pipes.setNumber(beatsin16(13,0,pipesMax));

	hsv2rgb_rainbow(CHSV( gHue, 255, 192),rgb);
  pipes.setPipeColour(rgb);
	pipes.writeFade(FADE_DEC);
}

void juggle() {
	byte dothue = 0;
	CRGB rgb;
	for( int i = 0; i < NUM_PIPES; i++) {
		pipes.setPipeNumber( i,beatsin16(i+7,0,9) );
		hsv2rgb_rainbow(CHSV(dothue, 200, 255),rgb);
    pipes.setPipeColour(i,rgb);
		dothue += 32;
	}
  pipes.writeFade(128);
}

