#include <NewPing.h>
#include <Adafruit_NeoPixel.h>
#define PIN 6
#define NUMPIXELS 8
#define TRIG_PIN 5
#define ECHO_PIN 18
#define MAX_DISTANCE 200

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  strip.begin();
  strip.setBrightness(20); // Set overall brightness to 50 out of 255 
  strip.show();            // (for lab can remove this, but my eyes pain)
}
void loop() {
  unsigned int echoUs = sonar.ping_median(5);
  float distance2 = sonar.convert_cm(echoUs);
  Serial.printf("Distance: %.2f cm\n", distance2);

  if (echoUs == 0) {
    Serial.println("No echo detected");
    strip.clear();
    strip.show();
    delay(500);
    return;
  }

  if(distance2 < 30 && distance2 > 0) {
    for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(128, 0, 0)); // red
    }
  }
  else if(distance2 >= 30 && distance2 <= 100) {
    for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(128, 128, 0)); // yellow
    }
  }
  else if(distance2 > 100) {
    for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 128, 0)); // green    
    }
  }

  strip.show(); 
  delay(500);  // Wait before starting the next loop
}