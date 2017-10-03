//  =================================================
//  This program runs on the Raspberry Pi
//  Purpose: Log doppler data via serial port and save to file
//  for Teensy 3.2 sending through FTDI-USB serial to Raspberry Pi
//
//  There is an apparent bug causing USB timeout after long (hours?) periods of
//  inactivity in this cheap (fake) ebay FTDI USB device.    
//  When it does shut down, the end of `dmesg` looks like this:
//  [49298.492956] ftdi_sio ttyUSB0: usb_serial_generic_read_bulk_callback - urb stopped: -32
//  [49476.022827] ftdi_sio ttyUSB0: usb_serial_generic_read_bulk_callback - urb stopped: -32
//
//  J.Beale 02-OCT-2017

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>     // strncat() etc.
#include <termios.h>
#include <unistd.h>
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */
#include <sys/time.h>   // gettimeofday()

#define MAXTIMEMS (300000)   // quit after this long
#define MAXLINE 100    // longest legal # of characters coming in serial port be                                                                             fore a newline
#define BASENAME "/mnt/usb1/doppler/log_"   // working directory & file prefix



#define error_message printf

int
set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                error_message ("error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                error_message ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

void
set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                error_message ("error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                error_message ("error %d setting term attributes", errno);
}



// ==============================================================
int main(int argc, char** argv) {

  time_t rawtime;
  struct tm * timeinfo;
  struct tm *tm;
  double  elapsedTime;
  char tbuf [80];

  struct timeval t1, t2;  // for gettimeofday()
  int i, n;
  FILE *fp;   // for output log file
  int fd;     // file descriptor for serial port

  char *portname = "/dev/ttyUSB0";
  char fname[120];
  char tod[80];
  int doflush = 0;

gettimeofday(&t1, NULL);  // start time
if ((tm = localtime(&t1.tv_sec)) == NULL) return -1;
strftime(tod, sizeof tod, "%Y%m%d_%H%M%S.csv",tm);

strcpy(fname,BASENAME);
strncat(fname,tod,strlen(tod));
printf("Opening %s\n",fname);
// return 0;

fp = fopen(fname,"w");
if (fp == NULL) return -1;

fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);

if (fd == 0)
{
        error_message ("error %d opening %s: %s", errno, portname, strerror (err                                                                             no));
        return;
}

set_interface_attribs (fd, B57600, 0);  // set speed to 57600 bps, 8N1 (no parit                                                                             y)
// set_blocking (fd, 0);                // set no blocking
set_blocking (fd, 1);                // set blocking

  char c;
  char buf [MAXLINE+1];
  while (1) {

    i = 0;
    doflush = 0;
    do {
        n = read (fd, &c, 1);  // read one character
        if (n > 0) {
           // putchar(c);
           // printf("%02x ",c);
           buf[i++] = c;
        }
    } while (c != 0x0a && n > 0);
    buf[i] = 0x00;  // null-terminate. Last chars either '0d 0a' but sometimes '                                                                             0a 0a' (!?)

    if (i > 1) { // ignore single-character lines (eg. single '0a')

      if ( (buf[0]=='#') && (buf[1]=='#') ) doflush=1;  // write to file at end                                                                              of each event

      // printf("Line: %s\n",buf);

      time (&rawtime);
      timeinfo = localtime (&rawtime);
      strftime (tbuf,80,"%F %T",timeinfo);

      // fprintf(fp,"%s, %d, %s",tbuf,strlen(buf),buf);   // write to file
      fprintf(fp,"%s, %s",tbuf,buf);   // write to file
      if (doflush==1) fflush(fp);

      gettimeofday(&t2, NULL);
      elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
      elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0; // us to msec
      if (elapsedTime > MAXTIMEMS) {  // exit if exceeded timeout in msec
        // printf("Time = %5.3f\n",elapsedTime);

        printf("Closing %s\n",fname);
        fprintf(fp,"# end\n");
        fflush(fp);
        fclose(fp);
        gettimeofday(&t1, NULL);  // start time
        if ((tm = localtime(&t1.tv_sec)) == NULL) return -1;
        strftime(tod, sizeof tod, "%Y%m%d_%H%M%S.csv",tm);
        strcpy(fname,BASENAME);
        strncat(fname,tod,strlen(tod));
        printf("Opening %s\n",fname);

        fp = fopen(fname,"w");
        if (fp == NULL) return -1;
      } // end if (i > 1)
    }
    // c = 0x20;  // ASCII space
    // write (fd, &c, 1);  // write one char. attempted bugfix for fake FTDI, but doesn't help
  } // while()

} // main
