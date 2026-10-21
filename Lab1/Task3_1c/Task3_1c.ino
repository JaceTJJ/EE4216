#include <NewPing.h>

#define TRIG_PIN 5
#define ECHO_PIN 18
#define MAX_DISTANCE 200

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}
void loop() {
  unsigned long startTime1 = micros();
  unsigned int distance = sonar.ping_cm();
  unsigned long endTime1 = micros();

  // calculate time taken to read echo pulse
  unsigned long duration1 = endTime1 - startTime1;

  Serial.printf("Ping: %u cm\n", distance);
  Serial.printf("Duration 1: %.2f ms\n", duration1 / 1000.0);

  // takes median time out of 5 measurements to improve accuracy
  unsigned long startTime2 = micros();
  unsigned int echoUs = sonar.ping_median(5);

  // program only goes to this line after ALL 5 echo pulses have been read
  unsigned long endTime2 = micros(); 

  unsigned long duration2 = endTime2 - startTime2;
  
  float distance2 = sonar.convert_cm(echoUs);
  Serial.printf("Ping Median: %.2f cm\n", distance2);
  Serial.printf("Duration 2: %.2f ms\n", duration2 / 1000.0);

  delay(500);  // Wait before starting the next loop
}
