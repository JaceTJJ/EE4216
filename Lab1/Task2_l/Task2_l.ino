#include <Adafruit_NeoPixel.h>
#define LED_PIN GPIO_NUM_38  // On board RGB LED is GPIO 38

// Define the number of LEDs in the strip. For a single built-in RGB LED, this is 1.
#define NUM_LEDS 1

// Create an instance of the Adafruit_NeoPixel class
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  
  // Initialize the NeoPixel library
  strip.begin();
  strip.setBrightness(20);
  strip.show(); // Initialize all pixels to 'off'
  Serial.println("Onboard LED blink starts");
}

void loop() {
  // Set the built-in RGB LED to white
  strip.setPixelColor(0, strip.Color(64, 64, 64)); // White color
  strip.show();
  delay(1000); // Wait for 1 second
  
  // Set the built-in RGB LED to red
  strip.setPixelColor(0, strip.Color(255, 0, 0)); // Red color
  strip.show();
  delay(1000); // Wait for 1 second

  // Set the built-in RGB LED to green
  strip.setPixelColor(0, strip.Color(0, 255, 0)); // Green color
  strip.show();
  delay(1000); // Wait for 1 second

  // Set the built-in RGB LED to blue
  strip.setPixelColor(0, strip.Color(0, 0, 255)); // Blue color
  strip.show();
  delay(1000); // Wait for 1 second

  // Turn off the built-in RGB LED
  strip.setPixelColor(0, strip.Color(0, 0, 0)); // Off
  strip.show();
  delay(1000); // Wait for 1 second
}
