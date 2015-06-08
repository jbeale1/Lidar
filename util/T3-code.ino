// Get range data via I2C from LIDAR-LITE at maximum speed (around 100 Hz, but varies)
// and calculate min, average, and max values. Report only when distance has shifted by X amount.
// send data out Serial (USB) to Raspberry Pi, and Serial1 port (TTY-level UART) to Serial-LCD display
// Arduino style code for Arduino 1.6.1 / Teensyduino 1.21 on Teensy T3.1 board

//  PaulS's code: http://forum.pjrc.com/threads/28036-Translating-Lidar-Lite-I2C-example-to-Teensy
// programmed into T3 on June 6 2015 J.Beale

#include <Wire.h>
#include <stdio.h> // for function sprintf

#define LIDARLite_ADDRESS 0x62 // Default I2C Address of LIDAR-Lite.
#define RegisterMeasure 0x00 // Register to write to initiate ranging.
#define MeasureValue 0x04 // Value to initiate ranging.
#define RegisterHighLowB 0x8f // Register to get both High and Low bytes in 1 call.

#define HWSERIAL Serial1
#define DELAY_USEC 150
#define CMAX 10 // how many I2C readings before one serial output to LCD
//#define CTHRESH 75  // how many cm difference from average needed to report reading
#define CTHRESH 25  // how many cm difference from average needed to report reading
// #define MINIMUM 80  // ignore any reading less than this 
#define MINIMUM 30  // ignore any reading less than this 
#define ZCNTMAX 1   // how many below-MINIMUM in a row to suppress

int reading = 0;
long dT; // difference in time between readings
long tNew, tOld;  // time in milliseconds()
long dTmax = 0;
long rCount = 0;  // how many readings since last printed
int first = 1;
boolean motion = false;

char cbs[16];  // character output string for 16x2 LCD display

float favg, favg1, fminv, fmaxv;
float f = 0.4; // average smoothing filter constant
float fm = 0.002;  // very slow averaging when motion detected
float f1 = 0.001; // peak-hold filter constant
int davg;
int dmin;
int dmax;
int dlast;
int count = 0;  // count readings before HWSERIAL output
int zeroCount = 0;  // how many "0" consecutive distance readings

void setup()
{
  
  Wire.begin();  // basic I2C library
  Serial.begin(115200); // start serial communication at 9600bps
  Serial1.begin(9600); // logic-level UART Serial1 (T3.1: digital pin 1)
  // while (!Serial) ;  // needed for R-Pi to work after reboot?
  
  delay(3000);
  Serial.println("LIDAR Test v.3 2015-06-04");
  HWSERIAL.println("LIDAR Test v.3 2015-06-04");
  tOld = millis();
  dmin = 1000;
  dmax = 0;
  dlast = 0;  // previous reading
  fminv = dmin;
  fmaxv = dmax;
}

void loop()
{
  int r;
  float change;  // difference between this reading and running average

  while (1) {
    //Serial.print('*');
    //Serial.flush();
    Wire.beginTransmission(LIDARLite_ADDRESS); // transmit to LIDAR-Lite
    Wire.write(RegisterMeasure);
    Wire.write(MeasureValue);
    r = Wire.endTransmission();
    if (r == 0) break;
    delayMicroseconds(DELAY_USEC);
  }

  while (1) {
    //Serial.print('%');  // locks up here
    //Serial.flush();
    Wire.beginTransmission((int)LIDARLite_ADDRESS); // transmit to LIDAR-Lite
    Wire.write((int)RegisterHighLowB); // sets register pointer to (0x8f)
    r = Wire.endTransmission();
    if (r == 0) break;
    delayMicroseconds(DELAY_USEC);
  }

  // request 2 bytes from LIDAR-Lite
  while (1) {
    //Serial.print('>');
    //Serial.flush();
    r = Wire.requestFrom((int)LIDARLite_ADDRESS, 2);
    if (r > 0) break;
    delayMicroseconds(DELAY_USEC);
  }

  if (Wire.available() >= 2) { // if two bytes were received
    reading = Wire.read() * 256;
    reading |= Wire.read();
  
    if ((reading < MINIMUM) && (zeroCount < ZCNTMAX)) zeroCount++;
    else zeroCount == 0;  
    
    //Serial.print(zeroCount);
// ----------------------------------------------
    if ((zeroCount == 0) || (reading >= MINIMUM)) {  // either this looks valid, or we've seen ZCNTMAX in a row already
     tNew = millis();
     dT = (tNew - tOld);
     if (first == 1) first = 0;
       else {
        if (dT > dTmax) dTmax = dT;
       }  

     davg = int(favg);
     dmin = int(fminv);
     dmax = int(fmaxv);
     
     sprintf(cbs, "%04d,%04d,%04d", dmin, davg, dmax);      
     change = reading - dlast;
     rCount++;
     if (abs(change) > CTHRESH) {
       motion = true;              // we have detected a new object in view
       dlast = reading;  // remember most recent printed reading
       Serial.print(reading);
       Serial.print(", ");
       Serial.print(rCount);
       Serial.print(", ");
       Serial.print(cbs);
       Serial.println();
       rCount = 0;
     } else {
       motion = false; 
     } 

     favg = f * reading + ((1.0-f) * favg);  // compute new running average quickly
     favg1 = fm * reading + ((1.0-fm) * favg1);  // compute new running average slowly
     
     if (reading != 0) {
       if (reading < fminv) fminv = reading;
        else  fminv = f1 * favg + ((1.0-f1) * fminv);  // leaky min-hold 
     }
    
     if (reading > fmaxv) fmaxv = reading;
      else fmaxv = f1 * favg + ((1.0-f1) * fmaxv);  // leaky peak-hold 

     if (1) {
       // Serial.println(cbs);
       if (count++ > CMAX) {
         HWSERIAL.println(cbs);
         count = 0;
       }

     }
     tOld=tNew;
    } // if zeroCount == 0
    
  } // if wire_available
  // else Serial.print(Wire.available());  // how many bytes were available?

  // lidar lite gives wrong results without this delay... why?
  delayMicroseconds(250);
}
