// Analog input test for Teensy 3.0    Oct 4 2012 - Feb 26 2014 J.Beale
// Setup: https://picasaweb.google.com/109928236040342205185/Electronics#5795546092126071650
// mods for low-freq audio input 18-MAR-2018 J.Beale

#define VREF (3.266)         // ADC reference voltage (= power supply)
// #define VINPUT (2.176)       // ADC input voltage from resistive divider to VREF
#define ADCMAX (65535)       // maximum possible reading from ADC
#define EXPECTED (ADCMAX*(VINPUT/VREF))     // expected ADC reading
#define SAMPLES (40000)      // how many samples to combine for pp, std.dev statistics

const int analogInPin = A2;  // Analog input pin A1 (Teensy3 pin 16)
const int LED1 = 13;         // output LED connected on Arduino digital pin 13

int sensorValue = 0;        // value read from the ADC input
long oldT;

void setup() {    // ==============================================================
      pinMode(LED1,OUTPUT);       // enable digital output for turning on LED indicator
      analogReference(INTERNAL);  // set analog reference to internal ref (?)
      analogReadRes(16);          // Teensy 3.0: set ADC resolution to this many bits
      analogReadAveraging(16);    // average this many readings
     
      Serial.begin(115200);       // baud rate is ignored with Teensy USB ACM i/o
      digitalWrite(LED1,HIGH);   delay(1000);   // LED on for 1 second
      digitalWrite(LED1,LOW);    delay(3000);   // wait for slow human to get serial capture running

      Serial.println("sec, avg, pk, std");     
      Serial.print("# Teensy 3.2 ADC 18-MAR-2018 JPB  Sample Avg: ");
      Serial.println(SAMPLES);
} // ==== end setup() ===========

void loop() {  // ================================================================ 
     
      long datSum = 0;  // reset our accumulated sum of input values to zero
      int sMax = 0;
      int sMin = 65535;
      long n;            // count of how many readings so far
      double x,mean,delta,sumsq,m2,variance,stdev;  // to calculate standard deviation
     
      oldT = millis();   // record start time in milliseconds

      sumsq = 0; // initialize running squared sum of differences
      n = 0;     // have not made any ADC readings yet
      mean = 0; // start off with running mean at zero
      m2 = 0;
     
      for (int i=0;i<SAMPLES;i++) {
        x = analogRead(analogInPin);
        datSum += x;
        if (x > sMax) sMax = x;
        if (x < sMin) sMin = x;
              // from http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
        n++;
        delta = x - mean;
        mean += delta/n;
        m2 += (delta * (x - mean));
      } 
      variance = m2/(n-1);  // (n-1):Sample Variance  (n): Population Variance
      stdev = sqrt(variance);  // Calculate standard deviation

      // Serial.print("# Samples/sec: "); // T3.2: 14630 Hz samples 3/18/2018 jpb
      // long durT = millis() - oldT;
      // Serial.print((1000.0*n/durT),2);
      float sec = millis() / 1000.0;
      float datAvg = (1.0*datSum)/n;
      Serial.print(sec,1);  // elapsed seconds
      Serial.print(" , ");     
      Serial.print(datAvg,1); // Average
     // Serial.print(" Offset: ");  Serial.print(datAvg - EXPECTED,2); // Offset
      Serial.print(" , ");  Serial.print(sMax-sMin); // Pk-Pk amplitude
      Serial.print(" , ");  Serial.println(stdev,3); // Standard Deviation
} // end main()  =====================================================
