// mdet.c - motion detection logger
// reports date/time to millisecond resolution of GPIO pin change
// gcc -Wall -pthread -o a.out mdet.c -lpigpio -lrt -lm
// sudo ./a.out

#include <stdio.h>
#include <pigpio.h>
#include <string.h>     // strncat() etc.#include <stdlib.h>
#include <unistd.h>
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */
#include <sys/time.h>   // gettimeofday()
#include <math.h>       // lrint()
#include <stdbool.h>    // type bool

#define SIGIN 22
#define BASENAME "/mnt/usb1/RCWL/log_"   // working directory & file prefix
#define TRUE (1)
#define FALSE (0)

struct timeval t2; // for gettimeofday()
volatile bool fresh;  // true when new value in t2
volatile uint32_t lastTick=0;
volatile int g_level = 0;
volatile uint32_t g_tick = 0;

void alert(int gpio, int level, uint32_t tick)
{
   gettimeofday(&t2, NULL);
   g_tick = tick;
   g_level = level;
   fresh = TRUE;
}

int main(int argc, char *argv[])
{
char fname[120];
FILE *fp;   // for output log file
struct tm *tm;
struct tm * timeinfo;
struct timeval t1;
char tod[80];         // time of day string
char tbuf[80];
int millisec;

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
   gpioSetPullUpDown(SIGIN, PI_PUD_UP);
   gpioSetAlertFunc(SIGIN, alert);

   printf("# Motion log start...\n");
   do {
      if (fresh == TRUE) {
        fresh = FALSE;
        millisec = lrint(t2.tv_usec/1000.0); // round to nearest millisec
        if (millisec>=1000) { // Allow for rounding up to nearest second
          millisec -=1000;
          t2.tv_sec++;
        }

        timeinfo = localtime(&t2.tv_sec);
        strftime(tbuf, 80, "%Y-%m-%d %H:%M:%S",timeinfo);
        fprintf(fp,"%s.%03d, ",tbuf,millisec);   // write to file including milliseconds
        fprintf(fp,"%d, %u, %d\n", g_level, g_tick, (g_tick-lastTick));
        lastTick = g_tick;
        fflush(fp);  // make sure it's written to the file
      }
      sleep(1);
   }   while (1);

   gpioTerminate();  // never reached
   fclose(fp);
}

