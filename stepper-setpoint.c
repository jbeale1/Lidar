// Arduino code
// drive a stepper motor (position ~ Vin) using control library
// for TMC2100 stepper driver board, JK42HS40-1304F motor
// Note: 1 full revolution = 8000 steps
// added "idle mode" moving back and forth slightly after control timeout
// J.Beale 31-OCT-2017

#include <AccelStepper.h>

#define ANALOG_IN A0    // potentiometer input
#define STEPFULLREV (8000)       // this many steps in 1 full revolution of stepmotor
#define MINDELTA (0.15)  // minimum stepsize that's "real" (debounce / avoid dither)
#define IDLE_LIMIT (3500)  // after no motion for this many loops, move back & forth
#define IDLERANGE (4000) // range of motion in counts during idle
#define IDLESPEED (0.2) // speed of motion in counts per cycle during idle

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
  stepper.setMaxSpeed(1500);
  stepper.setAcceleration(1500);  // steps per second per second
  stepper.moveTo(oldPos);
  Serial.begin(57600);
}

float ap = 0.75;  // filter constant
float dp2 = 0.01;  // longer lowpass filter constant
float afilt = 0;  // low-pass filtered value of analog in
float dfilt2 = 0;  // low-pass filtered value of delta step

unsigned int idleTime = 0;   // how many loop cycles since we last moved
float idlePos = 0.0;
float idleInc = +IDLESPEED;


// ====================================================
int idleIC = 0;
long int loopCount = 0;
void loop()
{
  int deltaStep;
    loopCount++;
    stepper.run();

    int analog_in = analogRead(ANALOG_IN); // 0..1023
    afilt = (afilt * (1.0-ap)) + (ap * analog_in);  // lowpass filter
    int newPos = (afilt * 8)-4092;    
    newPos = newPos % STEPFULLREV;            // wrap around to 0 at 360 degrees
    
    stepper.run();

    deltaStep = (newPos - oldPos);
    dfilt2 = (dfilt2 * (1.0-dp2)) + (dp2 * deltaStep);  // very lowpass filter
    if (abs(dfilt2) > MINDELTA) {
      Serial.print(deltaStep);  // debug output showing idle time
      Serial.print("   ");  // debug output showing idle time
      Serial.println(dfilt2);  // debug output showing idle time
      stepper.moveTo(newPos);
      idleTime = 0;
      idlePos = newPos;  // reset idle timeout position
    } else {
      idleTime++;
    }

    stepper.run();

    if (idleTime > IDLE_LIMIT) {
      idlePos+= idleInc;
      if (idleIC++ > IDLERANGE) {
        idleInc = -idleInc;
        idleIC = 0;  // idle increment counter
      }
      int idlePosI = int(idlePos) % STEPFULLREV;  // wrap around at 360 degrees
      stepper.moveTo(idlePosI);
    }

    stepper.run();
    if (abs(stepper.distanceToGo()) < 6) {
      digitalWrite(led, 1);  // turn on LED when setpoint reached
    } else {
      digitalWrite(led, 0);
    }

    oldPos = newPos;  // remember for next time
    for (int i=0;i<10;i++) {
      stepper.run();
      delayMicroseconds(100);
    }
    
    if ((loopCount % 100) == 0) {
      Serial.print(idleTime);  // debug output showing idle time
      Serial.print(", ");  // debug output showing idle time
      Serial.print(idlePos);  // debug output showing idle time
      Serial.println();  // debug output showing idle time
    }
}
