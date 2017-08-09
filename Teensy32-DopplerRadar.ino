// FFT to select freq. bins from signal on Teensy 3.2 pin A2 (ADC input)
// Set OUT_ALARM pin high if signal in some freq. bins over threshold
// Thanks to Teensy forum users: neutronned and Frank B.
// by J.Beale 9-AUG-2017

#include <Audio.h>

AudioInputAnalog         adc1;       
AudioAnalyzeFFT1024      myFFT;
AudioConnection  patchCord1(adc1, myFFT);

#define A_GAIN (5.0)    // gain applied to analog input signal
#define LED1 (13)       // onboard signal LED
#define OUT_ALARM (3)   // alarm signal (motion detected)
#define BINS 12         // how many separate frequency bins to work with

float level[BINS];  // frequency bins, current smoothed value
float avg[BINS];  // freq bins, long-term average
int alarm[BINS];  // by what factor each bin exceeds threshold (0 = no alarm)
boolean sumAlarm;  // true if any of the 'alarm' bins are true

static float fa = 0.10;  // smoothing fraction for low-pass filter
static float fb = 0.0005;  // smoothing fraction for long-term avg filter
static float aThresh = 2.5;      // factor by which current value must exceed long-ter avg. for alarm

int loops = 0;   // how many times through loop
boolean motion = false;  // if motion detected

void setDACFreq(int freq) {  // change sampling rate of internal ADC and DAC
const unsigned config = PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE | PDB_SC_DMAEN;
    PDB0_SC = 0; //<--add this line
    PDB0_IDLY = 1;
    PDB0_MOD = round((float)F_BUS / freq ) - 1;    
    PDB0_SC = config | PDB_SC_LDOK;
    PDB0_SC = config | PDB_SC_SWTRIG;
    PDB0_CH0C1 = 0x0101;    
}

void setup() {
int i;

  AudioMemory(12);
  AudioNoInterrupts();
  setDACFreq(11025);
  myFFT.windowFunction(AudioWindowHanning1024);
  AudioInterrupts();
  
  Serial.begin(115200);
  digitalWrite(LED1, false);
  pinMode(LED1, OUTPUT);
  digitalWrite(OUT_ALARM, false);
  pinMode(OUT_ALARM, OUTPUT);
  digitalWrite(LED1, true);
  digitalWrite(OUT_ALARM, true);
  delay(1000);

  Serial.print("min");
  Serial.print(",");
  Serial.print("alm");
  for (i=0; i<BINS; i++) {  // print column headers
      Serial.print(",");
      Serial.print("f");
      Serial.print(i);
  }
  Serial.println();
  Serial.println("# Doppler Microwave FFT on Teensy 3.1  5-AUG-2017");

  digitalWrite(LED1, false);
  digitalWrite(OUT_ALARM, false);


  for (i = 0; i<4; i++) do {} while (!myFFT.available());  // wait for FFT values to be ready  
  updateLevel(1.0);  // initialize frequency values with instantaneous ones
  updateAverage(1.0);  // initialize long-term average

  for (i=0; i<100; i++) {  // do a few rounds of averaging
    while (!myFFT.available());
    updateLevel(fa);  // start averaging frequency values 
    updateAverage(fa/2.0);  // start long-term averaging
  }
    
} // end setup()

#define FSTEP(a,b,f) ( (1.0-f)*a + (f*b) )  // interpolate between (a,b) by fraction f

void updateLevel(float f) {
    level[0] =  ((1.0-f)*level[0]) + f * myFFT.read(0) * 0.5 * A_GAIN;
    level[1] =  ((1.0-f)*level[1]) + f * myFFT.read(1) * 1.5 * A_GAIN;
    level[2] =  ((1.0-f)*level[2]) + f * myFFT.read(2) * 2 * A_GAIN;
    level[3] =  ((1.0-f)*level[3]) + f * myFFT.read(3) * 3.5 * A_GAIN;
    level[4] =  ((1.0-f)*level[4]) + f * myFFT.read(4, 5) * 5 * A_GAIN;
    level[5] =  ((1.0-f)*level[5]) + f * myFFT.read(6, 8) * 7 * A_GAIN;
    level[6] =  ((1.0-f)*level[6]) + f * myFFT.read(9, 13) * 9 * A_GAIN;
    level[7] =  ((1.0-f)*level[7]) + f * myFFT.read(14, 22) * 11.0 * A_GAIN;
    level[8] =  ((1.0-f)*level[8]) + f * myFFT.read(23, 40) * 15 * A_GAIN;
    level[9] =  ((1.0-f)*level[9]) + f * myFFT.read(41, 66) * 15 * A_GAIN;
    level[10] = ((1.0-f)*level[10]) + f * myFFT.read(67, 93) * 18 * A_GAIN;
    level[11] = ((1.0-f)*level[11]) + f * myFFT.read(94, 131) * 10 * A_GAIN;
}

// parameter f is long-term smoothing fraction (f very small => very long-term avg.)
void updateAverage(float f) {   // find long-term avg, and "alarm" bins
int afac;
  
    sumAlarm = false;
    for (int i=0; i<BINS; i++) {
      avg[i] = FSTEP(avg[i], level[i], f);  // update long-term averages
      afac = level[i] / (aThresh * avg[i]);
      if (afac > 9) afac = 9;
      alarm[i] = afac;
      if (afac > 0) {
        sumAlarm = true;
      }
      else
        alarm[i] = false;
    }
}
 
void loop() {
  if (myFFT.available()) {
    updateLevel(fa);  // get current smoothed frequency values
    updateAverage(fb);    // get long-term-aveage values
    
    if (sumAlarm) {
      digitalWrite(OUT_ALARM, true);
    } else {
      digitalWrite(OUT_ALARM, false);
    }

    if ((sumAlarm) && ((loops++)%5 == 0)) {
      for (int i=0; i<BINS; i++) {
        Serial.print(alarm[i]);
        Serial.print(",");
      }
     Serial.print("  ");  // extra space for separation
     for (int i=0; i<BINS; i++) {
      Serial.print(level[i]);
      if (i < (BINS-1)) Serial.print(",");
     }
     Serial.println();
    }
  }
  
} // end main loop()

