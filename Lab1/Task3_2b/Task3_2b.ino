#include <Adafruit_NeoPixel.h>
#define PIN 6
#define NUMPIXELS 8
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.clear(); // set all LEDs to off

  strip.setBrightness(50); // Set overall brightness to 50 out of 255 (for lab can remove this, but my eyes pain)

  strip.setPixelColor(0, strip.Color(128, 128, 128)); // rgb
  strip.setPixelColor(2, strip.Color(128, 0, 0)); // brightness will be 50/255 * 128
  strip.setPixelColor(4, strip.Color(0, 128, 0)); 
  strip.setPixelColor(6, strip.Color(0, 0, 128)); 

  strip.show(); // Update LED bar
}

void loop() {

}
