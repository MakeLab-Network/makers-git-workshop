#include <Servo.h>

Servo myServo;  // Create servo object

void setup() {
  myServo.attach(9);  // Attach the servo on pin 9
}

void loop() {
  myServo.write(90);  // Set servo to 90 degrees (middle position)
  delay(1000);        // Wait 1 second
  myServo.write(180); // Set servo to 180 degrees (max position)
  delay(1000);        // Wait 1 second
}
