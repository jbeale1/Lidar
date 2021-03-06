// Teensy 3.2: Low-freq audio input 3:28 pm 24-MAR-2018 J.Beale

#define VREF (3.266)         // ADC reference voltage (= power supply)
#define ADCMAX (65535)       // maximum possible reading from ADC
#define EXPECTED (ADCMAX*(VINPUT/VREF))     // expected ADC reading
#define SAMPLES (100)      // how many samples to combine for pp, std.dev statistics
#define RAWAVG 225  // how many samples to average together before stats step

#define BXAVG 3    // how many samples in boxcar for rolling average

unsigned int idx = 0;  // index into boxcar average
float rs[BXAVG];   // boxcar averaging

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
      Serial.print("# Teensy 3.2 ADC 21-MAR-2018 JPB  Sample Avg: ");
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

      delay(5);  // tune for close to 1 Hz update rate     
      for (int i=0;i<SAMPLES;i++) {
        long rawSum = 0;
        for (int j=0; j<RAWAVG; j++) {
          rawSum += analogRead(analogInPin);
        }
        x = rawSum / RAWAVG;
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
      // 146 sample avg => 100 Hz rate
      
      //Serial.print("# Samples/sec: "); // T3.2: 14630 Hz samples 3/18/2018 jpb
      //long durT = millis() - oldT;
      //Serial.print((1000.0*n/durT),2);
      
      float sec = millis() / 1000.0;
      float datAvg = (1.0*datSum)/n;
      float db = 20 * log10(stdev);  // not necessarily "real" dB
      rs[idx] = db;
      idx = (idx+1) % BXAVG;
      float dbavg = 0;
      for (int i=0; i<BXAVG; i++) {
        dbavg += rs[i];
      }
      dbavg /= BXAVG;
      Serial.print(sec,1);  // elapsed seconds
      Serial.print(" , ");     
      Serial.print(datAvg,1); // Average
     // Serial.print(" Offset: ");  Serial.print(datAvg - EXPECTED,2); // Offset
      Serial.print(" , ");  Serial.print(sMax-sMin); // Pk-Pk amplitude
      Serial.print(" , ");  Serial.println(dbavg,1); // log amplitude of signal
} // end main()  =====================================================
