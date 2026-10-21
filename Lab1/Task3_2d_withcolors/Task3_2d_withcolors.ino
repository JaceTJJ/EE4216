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
  strip.setBrightness(10); // Set overall brightness to 10 out of 255 
  strip.show();            // (for lab can remove this, but my eyes pain)
}

void loop() {
  // takes median time out of 5 measurements to improve accuracy
  unsigned int echoUs = sonar.ping_median(5);
  float distance2 = sonar.convert_cm(echoUs);
  Serial.printf("Distance: %.2f cm\n", distance2);

  // for distances that are not 0cm but measured as 0 because out of max range
  if (echoUs == 0) {
    Serial.println("No echo detected");
    strip.clear();
    strip.show();
    delay(500);
    return;
  }

  int leds_to_turn_on;
  uint32_t color;

  if (distance2 < 30) color = strip.Color(128, 0, 0); // Red
  else if (distance2 <= 70) color = strip.Color(128, 128, 0); // Yellow
  else color = strip.Color(0, 128, 0); // Green

  if (distance2 <= 10) leds_to_turn_on = 8;
  else if (distance2 >= 100) leds_to_turn_on = 1;
  else {
    float x = (distance2 - 10) / 90.0 * 7;
    leds_to_turn_on = 8 - round(x); // bc distance increases, no. of leds ON decreases
  }

  strip.clear();
  for (int i = 0; i < leds_to_turn_on; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show(); 
  delay(500);  // Wait before starting the next loop
}