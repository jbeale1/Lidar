// FFT to select freq. bins from signal on Teensy 3.2 pin A2 (ADC input)
// Set OUT_ALARM pin high if signal in some freq. bins over threshold
// Thanks to Teensy forum users: neutronned and Frank B.
// Tuned for signal from HB100 (10.525 GHz doppler radar)
// by J.Beale 31-AUG-2017  T3.2 96 MHz Faster

#include <Audio.h>

AudioInputAnalog         adc1;       
AudioAnalyzeFFT1024      myFFT;
AudioConnection  patchCord1(adc1, myFFT);

#define SERPORT Serial1

#define A_GAIN (9.0)    // gain applied to analog input signal
#define LED1 (13)       // onboard signal LED
#define OUT_ALARM (3)   // alarm signal (motion detected)
#define BINS 15         // how many separate frequency bins to work with
#define BIN_CUTOFF (3)    // ignore bins below this value
//#define MOTION_THRESH (10)  // sum of alarm channels reaches this value = motion detect
#define MOTION_THRESH (5)  // sum of alarm channels exceeding this value = motion detect
#define PEAK_THRESH (4)  // largest bin must reach this value
#define DECRATIO (4)    // decimation ratio (data processed / printed out)

#define UC unsigned char

float level[BINS];  // frequency bins, current smoothed value
float avg[BINS];  // freq bins, long-term average
UC alarm[BINS];  // by what factor each bin exceeds threshold (0 = no alarm)
UC hist[BINS];   // history buffer for early (pre-printout) motion alarm values

int sumAlarm;  // sum of all alarm[i] bins, 0 if no alarm
int iMax;      // index of bin with maximum 'Alarm' value
boolean motion;  // true if motion detected by alarm sum > MOTION_THRESH
boolean motionOld;  // motion value at previous timestep
boolean firstPrint; // if this is the first line of motion data printed out
int runLength;   // for how many consecutive readings "motion" has been detected

static float fa = 0.1;  // smoothing fraction for low-pass filter
static float fb = 0.0005;  // smoothing fraction for long-term avg filter
// static float aThresh = 0.17;      // amount by which current value must exceed long-ter avg. for alarm
static float aThresh = 0.3;      // amount by which current value must exceed long-ter avg. for alarm

unsigned int loops = 0;   // how many times through processing loop
unsigned int pcnt = 0;  // how many times through printout loop

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
  // setDACFreq(25245); // 11.025 kHz * (24.1/10.525) = 25.245 kHz
  myFFT.windowFunction(AudioWindowHanning1024);
  // analogReference(EXTERNAL);  // set ADC voltage reference to 3.3V external
  AudioInterrupts();
  
  SERPORT.begin(19200);
  digitalWrite(LED1, false);
  pinMode(LED1, OUTPUT);
  digitalWrite(OUT_ALARM, false);
  pinMode(OUT_ALARM, OUTPUT);
  digitalWrite(LED1, true);
  digitalWrite(OUT_ALARM, true);
  delay(1000);

  SERPORT.print("min");
  SERPORT.print(",");
  SERPORT.print("alm");
  for (i=0; i<BINS; i++) {  // print column headers
      SERPORT.print(",");
      SERPORT.print("f");
      SERPORT.print(i);
  }
  SERPORT.println();
  // SERPORT.print("# log(10.0) = ");
  // SERPORT.println(log(10.0),5);
  SERPORT.print("# Doppler Microwave FFT on Teensy 3.2 ver 1.22 28-AUG-2017");
  for (i=0; i<8; i++) {
    delay(1000);
    SERPORT.print(".");
  }
  SERPORT.println();
  SERPORT.print("# ");
  
  digitalWrite(LED1, false);
  digitalWrite(OUT_ALARM, false);
  runLength = 0;
  firstPrint = false;


  for (i = 0; i<4; i++) do {} while (!myFFT.available());  // wait for FFT values to be ready  
  updateLevel(1.0);  // initialize frequency values with instantaneous ones
  updateAverage(1.0);  // initialize long-term average

  for (i=0; i<100; i++) {  // do a few rounds of averaging
    while (!myFFT.available());
    SERPORT.print(".");
    updateLevel(fa);  // start averaging frequency values 
    updateAverage(fa/2.0);  // start long-term averaging
  }
  SERPORT.println("");
  SERPORT.println("# Starting now");
    
} // end setup()

#define FSTEP(a,b,f) ( (1.0-f)*a + (f*b) )  // interpolate between (a,b) by fraction f

void updateLevel(float f) {
    level[0] =  ((1.0-f)*level[0]) + f * (myFFT.read(0) * 0.5 * A_GAIN);
    level[1] =  ((1.0-f)*level[1]) + f * (myFFT.read(1) * 1.5 * A_GAIN);
    level[2] =  ((1.0-f)*level[2]) + f * (myFFT.read(2) * 2 * A_GAIN);
    level[3] =  ((1.0-f)*level[3]) + f * (myFFT.read(3) * 3.5 * A_GAIN);
    level[4] =  ((1.0-f)*level[4]) + f * (myFFT.read(4, 5) * 5 * A_GAIN);
    level[5] =  ((1.0-f)*level[5]) + f * (myFFT.read(6, 8) * 7 * A_GAIN);
    level[6] =  ((1.0-f)*level[6]) + f * (myFFT.read(9, 13) * 9 * A_GAIN);
    level[7] =  ((1.0-f)*level[7]) + f * (myFFT.read(14, 22) * 11.0 * A_GAIN);
    level[8] =  ((1.0-f)*level[8]) + f * (myFFT.read(23, 40) * 15 * A_GAIN);
    level[9] =  ((1.0-f)*level[9]) + f * (myFFT.read(41, 66) * 15 * A_GAIN);
    level[10] = ((1.0-f)*level[10]) + f * (myFFT.read(67, 93) * 18 * A_GAIN);
    level[11] = ((1.0-f)*level[11]) + f * (myFFT.read(94, 131) * 10 * A_GAIN);
    level[12] = ((1.0-f)*level[11]) + f * (myFFT.read(132, 250) * 6 * A_GAIN);
    level[13] = ((1.0-f)*level[11]) + f * (myFFT.read(251, 375) * 4 * A_GAIN);
    level[14] = ((1.0-f)*level[11]) + f * (myFFT.read(376, 511) * 4 * A_GAIN);
}

// ===================================================================================
// parameter f is long-term smoothing fraction (f very small => very long-term avg.)
void updateAverage(float f) {   // find long-term avg, and "alarm" bins
float afac;
float maxVal;
  
    sumAlarm = 0;
    iMax = 0;     // bin with peak value
    maxVal = 0;
    for (int i=0; i<BINS; i++) {
      avg[i] = FSTEP(avg[i], level[i], f);  // update long-term averages
      afac = log(level[i] / avg[i]) / (aThresh);  // recall units are logarithmic. log() = log base e
      if (afac < 1) afac = 0;
      if (afac > maxVal) {   // have we found a new maximum?
        maxVal = afac;
        iMax = i;
      }
      
      if (i >= BIN_CUTOFF) 
        sumAlarm += afac;         // sum: ignore first n bins (bkgnd noise), and don't clamp at '9'
      if (afac > 9) afac = 9;
      alarm[i] = afac;
    }
    motionOld = motion;  // remember value from previous timestep
    if ((sumAlarm >= MOTION_THRESH) && (maxVal >= PEAK_THRESH) && (iMax >= BIN_CUTOFF) ) {  // motion detected
      motion = true;
      runLength++;
    } else {  // motion not currently detected
      if ((motion) && (runLength > (2*DECRATIO)) )  // just ending an event
        SERPORT.println();  // leave blank line to separate events
      motion = false;
      firstPrint = true;
      runLength = 0;
    }
}

char buf[20];
// ===================================================================================
void loop() {
  int i;
  
  if (myFFT.available()) {
    updateLevel(fa);      // get current smoothed frequency values
    updateAverage(fb);    // get long-term-average values

    digitalWrite(OUT_ALARM, motion);
    // digitalWrite(LED1, motion);  // onboard LED current adds noise?
    if ((loops++)%DECRATIO == 0) {
      pcnt++;  // print-loop counter
      
      if (motion) {
        if ( runLength > (2*DECRATIO) )  {
          if (firstPrint) {  // if first line, also print previous saved line
            firstPrint = false;
            for (i=0; i<BINS; i++) {
              SERPORT.print(hist[i]);
              SERPORT.print(",");
            }
            SERPORT.println();            
          }
          for (i=0; i<BINS; i++) { // print alarm indexes
            SERPORT.print(alarm[i]);
            SERPORT.print(",");
          }
         SERPORT.print("  "); sprintf(buf,"%02d, ",runLength/DECRATIO); SERPORT.print(buf);  
         sprintf(buf,"%03d, ",sumAlarm);  SERPORT.print(buf);
         sprintf(buf,"%d, ",iMax);  SERPORT.print(buf);
         for (i=0; i<BINS; i++) {
          SERPORT.print(level[i]);
          if (i < (BINS-1)) SERPORT.print(",");
         }
         SERPORT.println();
        } // if (runlength > 2*dec..)
       
        else { // motion, but not yet 2x samples
               // firstPrint = true;
               for (i=0; i<BINS; i++) {         
                   hist[i] = alarm[i];  // back up for possible printout
               }
             } // else
      } // if (motion)
    } // if ((loops++ ...))
    
  } // if (myFFT.available...)
  
} // end main loop()

