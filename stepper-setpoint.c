// Arduino code
// drive a stepper motor using control library
// for TMC2100 stepper driver board, JK42HS40-1304F motor
// J.Beale 28-OCT-2017

#define ANALOG_IN A0    // potentiometer input

#include <AccelStepper.h>
// Define a stepper motor (driver board) and the pins it will use
AccelStepper stepper(AccelStepper::DRIVER, 9, 10);  // (step, direction)

static int led = 13;  // indicator LED on Arduino
int oldPos=0;           // previous position setpoint

void setup()
{  
  
  randomSeed(analogRead(0));
  pinMode(led, OUTPUT);  
  stepper.setEnablePin(8);  // to control stepper driver enable
  stepper.setPinsInverted(false, false, true);  // enable is low true (high = disable)
  stepper.setMaxSpeed(4000);
  stepper.setAcceleration(2000);  // steps per second per second
  stepper.moveTo(oldPos);
}

void loop()
{
    int analog_in = analogRead(ANALOG_IN); // 0..1023
    int newPos = (analog_in * 4)-2046;
    if (newPos != oldPos) {
      stepper.moveTo(newPos);
    }
    stepper.run();
    if (abs(stepper.distanceToGo()) < 6) {
      digitalWrite(led, 1);  // turn on LED when setpoint reached
    } else {
      digitalWrite(led, 0);
    }
    oldPos = newPos;  // remember for next time
}
