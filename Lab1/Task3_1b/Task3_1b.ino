#define TRIG_PIN 5
#define ECHO_PIN 18
void setup() {
  // Initialize the serial communication:
  Serial.begin(115200);
  // Configure the pins:
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}
void loop() {
  // Clear the trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Send a 10us pulse to the trigger pin. This causes the sensor to send out an ultrasonic burst.
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH); // Measure the duration of the echo pin's pulse in microseconds.
  float distance = (duration * 0.0343) / 2; // Calculate the distance in centimeters:
  Serial.printf("Duration: %.2f ms\n", duration / 1000.0);
  Serial.printf("Distance:  %.02fcm\n", distance);
  
  delay(500); // Add a delay before the next measurement
}
