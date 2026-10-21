#include <NewPing.h>
#include <Adafruit_NeoPixel.h>
#define PIN 6
#define NUMPIXELS 8

#define TRIG_PIN 5
#define ECHO_PIN 18
#define MAX_DISTANCE 400

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Initialize the serial communication:
  Serial.begin(115200);
  
  // Configure the pins:
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  strip.begin();
  strip.setBrightness(20); // Set overall brightness to 20 out of 255 (for lab can remove this, but my eyes pain)
  strip.show();
}
void loop() {
  // takes median time out of 5 measurements to improve accuracy
  unsigned int echoUs = sonar.ping_median(5);
  float distance2 = sonar.convert_cm(echoUs);
  Serial.print("Distance 2: ");
  Serial.print(distance2);
  Serial.println(" cm");

  // for distances that are not 0cm but measured as 0 because out of range
  if (echoUs == 0) {
  Serial.println("No echo detected");
  strip.clear();
  strip.show();
  delay(500);
  return;
  }

  int leds_to_turn_on;
  

  if (distance2 <= 10) leds_to_turn_on = 8;
  else if (distance2 >= 100) leds_to_turn_on = 1;
  else {
    float x = (distance2 - 10) / 90.0 * 7;
    leds_to_turn_on = 8 - round(x); // bc distance increases, no. of leds ON decreases
  }

  strip.clear();
  for (int i = 0; i < leds_to_turn_on; i++) {
    strip.setPixelColor(i, strip.Color(0, 128, 0));
  }

  strip.show(); 
  delay(500);  // Wait before starting the next loop
}