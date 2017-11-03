// mdet.c - motion detection logger
// reports date/time to millisecond resolution of GPIO pin change
// gcc -Wall -pthread -o a.out mdet.c -lpigpio -lrt -lm
// sudo ./a.out
// version 0.2 24-OCT-2017

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pigpio.h>
#include <string.h>     // strncat() etc.#include <stdlib.h>
#include <unistd.h>
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */
#include <sys/time.h>   // gettimeofday()
#include <math.h>       // lrint()
#include <stdbool.h>    // type bool

// #define SIGIN 22
#define SIGIN 17
#define BASENAME "/mnt/usb1/RCWL/log_"   // working directory & file prefix
#define STILLCAP "echo 'still' > /home/pi/pikrellcam/www/FIFO" //  command to capture still
#define VIDCAP "echo 'record on 4 4' > /home/pi/pikrellcam/www/FIFO"
#define SENDALARM "/home/pi/pikrellcam/scripts/jpb-alarm rp8 alarm"
#define TRUE (1)
#define FALSE (0)

struct timeval t2; // for gettimeofday()
volatile bool fresh;  // true when new value in t2
volatile uint32_t lastTick=0;
volatile int g_level = 0;
volatile uint32_t g_tick = 0;

// alert() called from GPIO interrupt saves current time in global t2
// also microsecond timer in g_tick and GPIO level in g_level
void alert(int gpio, int level, uint32_t tick)
{
   gettimeofday(&t2, NULL);
   g_tick = tick;
   g_level = level;
   fresh = TRUE;
}

/* Subtract the ‘struct timeval’ values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0. */

int
timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
} // end timeval_subtract

int main(int argc, char *argv[])
{
char fname[120];
FILE *fp;   // for output log file
struct tm *tm;
struct tm * timeinfo;
struct timeval t1, deltaT;
char tod[80];         // time of day string
char tbuf[80];
int millisec;
float deltaSec = 0;
int retval = 0;

   if (gpioInitialise()<0) {
    printf("Cannot initialize GPIO, quitting.\n");
    return 1;
   }

  fresh = FALSE;  // start out with no GPIO level changes yet seen
  gettimeofday(&t1, NULL);  // start time
  if ((tm = localtime(&t1.tv_sec)) == NULL) return -1;
  strftime(tod, sizeof tod, "%Y%m%d_%H%M%S.csv",tm);

  strcpy(fname,BASENAME);
  strncat(fname,tod,strlen(tod));
  printf("Opening %s\n",fname);

  fp = fopen(fname,"w");
  if (fp == NULL) return -1;

  gpioSetMode(SIGIN, PI_INPUT);
  // gpioSetPullUpDown(SIGIN, PI_PUD_UP);
  gpioSetPullUpDown(SIGIN, PI_PUD_DOWN);
  gpioSetAlertFunc(SIGIN, alert);

  printf("# Motion log start...\n");
  do {
      if (fresh == TRUE) {
        fresh = FALSE;
        timeval_subtract (&deltaT, &t2, &t1);
        if (g_level == 1) {         // is this rising edge (start of new motion event?)
          t1.tv_sec = t2.tv_sec;    // save time structure values for next time
          t1.tv_usec = t2.tv_usec;
        }

        millisec = lrint(t2.tv_usec/1000.0); // round to nearest millisec
        if (millisec>=1000) { // Allow for rounding up to nearest second
          millisec -=1000;
          t2.tv_sec++;
        }

        timeinfo = localtime(&t2.tv_sec);
        strftime(tbuf, 80, "%Y-%m-%d %H:%M:%S",timeinfo);
        fprintf(fp,"%s.%03d, ",tbuf,millisec);   // write to file including milliseconds
        if (g_level == 0) { // returned to zero; end of motion event
           deltaSec = (g_tick-lastTick) / 1E6;  // ticks in microseconds
           fprintf(fp,"%d, %4.1f\n", g_level, deltaSec);
        } else {  // start of new motion event
           deltaSec = deltaT.tv_sec + (deltaT.tv_usec/1.0E6);
           fprintf(fp,"%d,     %4.1f\n", g_level, deltaSec);
           retval = system(STILLCAP);  // command to trigger local capture of still image
           retval |= system(SENDALARM); // notify other device of motion
        }
        lastTick = g_tick;
        fflush(fp);  // make sure it's written to the file
/*
        if (g_level == 1) {
            // capture still image
            // raspistill -t 1000 -rot 180 -o test.jpg
        tm = localtime(&t2.tv_sec);
        strftime(tod, sizeof tod, "m_%Y%m%d_%H%M%S.jpg",tm);
        strcpy(fname, "raspistill -t 1000 -rot 180 -o "); // command to record image
        strcat(fname, tod);
        printf("Saving %s\n", tod);
        retval = system(fname);
        }
*/

      } // if fresh
      sleep(1);
   }   while (1);

  gpioTerminate();  // never reached
  fclose(fp);
  return retval;
}
