#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>

#include "database.h"

#define IDLE 0
#define GET_COMMAND 1
#define BUILD_CAN_FRAME 2

#define uint unsigned long


static volatile int ctrlC = 0;
int fd;
int rx_state;
int rx_step;
unsigned long timestamp;
unsigned long ID;
int len;
unsigned char data[8];

// Mode. Interpret mode, screen LSB nibble=log mode - next nibble=inq frames - 0x100=force sceen toggle
int mode = 0x01;

// non arduino
unsigned long int micros() {
  return 0;
}

void ledToggle () {
}

// prototype
void toConsole (char *buf);

//Copied the CAN_FRAME structure from the DUe_can library here (including the .h gave too many errors in x86 gcc)

//This is architecture specific. DO NOT USE THIS UNION ON ANYTHING OTHER THAN THE CORTEX M3 / Arduino Due
//UNLESS YOU DOUBLE CHECK THINGS!
typedef union {
    uint64_t value;
	struct {
		uint32_t low;
		uint32_t high;
	};
	struct {
        uint16_t s0;
		uint16_t s1;
		uint16_t s2;
		uint16_t s3;
    };
	uint8_t bytes[8];
	uint8_t byte[8]; //alternate name so you can omit the s if you feel it makes more sense
} BytesUnion;

typedef struct
{
	uint32_t id;		// EID if ide set, SID otherwise
	uint32_t fid;		// family ID
	uint8_t rtr;		// Remote Transmission Request
	uint8_t priority;	// Priority but only important for TX frames and then only for special uses.
	uint8_t extended;	// Extended ID flag
	uint8_t length;		// Number of data bytes
	BytesUnion data;	// 64 bits - lots of ways to access it.
} CAN_FRAME;



// some cleanup between C versions

#define boolean int
#define true -1
#define false 0

// some defines about where to split the sceen. Ensure these values when added do not exceed the screen size minus 2
#define screenBlock1 33
#define screenBlock2 14

#define PBUFSIZE        180 // size of buffer for sprintf

// sprintf buffer
char buf[PBUFSIZE];

// positions of frame ID's
typedef struct
{
} canId;


int idIndex[0xFFF];
//int idAge[0xFFF];
int lastidIndexCol = 1;

// Frame counter
unsigned long frameCount = 0;

char getch () {
  char buf = 0;
  struct termios old = {0};
  if (tcgetattr(0, &old) < 0)
    perror("tcsetattr()");
  old.c_lflag &= ~ICANON;
  old.c_lflag &= ~ECHO;
  old.c_cc[VMIN] = 0;
  old.c_cc[VTIME] = 0;
  if (tcsetattr(0, TCSANOW, &old) < 0)
    perror("tcsetattr ICANON");
  if (read(0, &buf, 1) < 0)
    perror ("read()");
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  if (tcsetattr(0, TCSADRAIN, &old) < 0)
    perror ("tcsetattr ~ICANON");
  return (buf);
}

void screenToggle() {
  snprintf(buf, PBUFSIZE, "%c[1;155f %03x %c", 27, mode, frameCount % 2 ? '-' : '|');
  toConsole (buf);
}

// direct access by CAN id to the database. The value represents either a position for the unknown display, or (when offset by 0x800) and index in the database
void initIndex () {
  int idx;
  int dbIdx;
  // initialize the index
  for (idx = 0; idx < 0xfff; idx++) {
     idIndex[idx] = 0;
     //idAge[idx] = 0;
  }
  //pre-add the known unknows
  lastidIndexCol = 1;
  for (dbIdx = 0; fixedPositions[dbIdx] != 0; dbIdx++) {
    idIndex[fixedPositions[dbIdx]] = lastidIndexCol++;
  }
  //pre-add the interpreter database, possibly overwriting an previously unknown entry
  for (dbIdx = 0; fieldPositions[dbIdx].id != 0; dbIdx++) {
    if (idIndex[fieldPositions[dbIdx].id] == 0) {
      idIndex[fieldPositions[dbIdx].id] = dbIdx + 0x800;
    }
  }
}

// print frame in log format
void logFrame(CAN_FRAME *frame) {
  snprintf(buf, PBUFSIZE, "%03lx %1d %02x %02x %02x %02x %02x %02x %02x %02x\r\n", (unsigned long int)frame->id, frame->length, frame->data.bytes[0], frame->data.bytes[1], frame->data.bytes[2], frame->data.bytes[3], frame->data.bytes[4], frame->data.bytes[5], frame->data.bytes[6], frame->data.bytes[7]);
  toConsole (buf);
}

void logFrameGVRET(CAN_FRAME *frame) {
  //124096491,0000017A,false,0,8,FF,FF,FF,AA,00,C0,31,A3,
  snprintf(buf, PBUFSIZE, "%ld,%08lx,%s,0,%1d,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,\r\n", micros(), (unsigned long int)frame->id, frame->extended ? "true" : "false", frame->length, frame->data.bytes[0], frame->data.bytes[1], frame->data.bytes[2], frame->data.bytes[3], frame->data.bytes[4], frame->data.bytes[5], frame->data.bytes[6], frame->data.bytes[7]);
  toConsole (buf);
}

void logFrameCRTD(CAN_FRAME *frame) {
  //1320745424.002 R11 402 FA 01 C3 A0 96 00 07 01
  int temp;
  snprintf(buf, PBUFSIZE, "%.6f R11 %03lX", micros() / 1000000.0, (unsigned long int)frame->id);
  toConsole (buf);
  for (temp = 0; temp < frame->length; temp++)
  {
    snprintf(buf, PBUFSIZE, " %02X", frame->data.bytes[temp]);
    toConsole (buf);
  }
  toConsole ("\r\n");
}

// print frame on screen
void printFrame(CAN_FRAME *frame, int l, int c) {
  int i;
  int j;
/*
  int age = (frameCount >> 5) - idAge[frame->id];
  if (age < 0) {
    age = 0;
    idAge[frame->id] = frameCount >> 5;
  } else if (age > 9) {
    age = 9;
    idAge[frame->id] = (frameCount >> 5) - 9;
  } */

//snprintf(buf, PBUFSIZE, "%c[%d;%df%1d %03lx %1d %02x %02x %02x %02x %02x %02x %02x %02x ", 27, l, c, age, (unsigned long int)frame->id, frame->length, frame->data.bytes[0], frame->data.bytes[1], frame->data.bytes[2], frame->data.bytes[3], frame->data.bytes[4], frame->data.bytes[5], frame->data.bytes[6], frame->data.bytes[7]);
//snprintf(buf, PBUFSIZE, "%c[%d;%df%03lx %1d %02x %02x %02x %02x %02x %02x %02x %02x ",     27, l, c,      (unsigned long int)frame->id, frame->length, frame->data.bytes[0], frame->data.bytes[1], frame->data.bytes[2], frame->data.bytes[3], frame->data.bytes[4], frame->data.bytes[5], frame->data.bytes[6], frame->data.bytes[7]);
  j = snprintf(buf, PBUFSIZE, "%c[%d;%df%03lx %1d ", 27, l, c,      (unsigned long int)frame->id, frame->length);
  for (i = 0; i < frame->length; i++) {
    snprintf(buf + j, PBUFSIZE - j, "%02x ", frame->data.bytes[i]);
    j += 3;
  }
  for (;i < 8; i++) {
    snprintf(buf + j, PBUFSIZE - j, "   ");
    j += 3;
  }
  toConsole (buf);
}

// get a specific bit string from the frame's data as long and normalize
long getVal (CAN_FRAME *frame, int index) {

  int startbit = fieldPositions[index].startbit;
  int endbit   = fieldPositions[index].endbit;

  int startbyte = startbit >> 3;
  int startbitt = startbit & 0x07;
  int endbyte   = endbit   >> 3;
  int endbitt   = endbit   & 0x07;
  unsigned long result = 0;

  if (startbitt > 0) {
    result = frame->data.bytes[startbyte] & (0xff>>startbitt);
  } else {
    result = frame->data.bytes[startbyte];
  }

  startbyte++;
  while (startbyte <= endbyte) {
    result = (result << 8) | frame->data.bytes[startbyte];
    startbyte++;
  }

  if (endbitt < 7) {
    result = result >> (7 - endbitt);
  }

  result = result - fieldPositions[index].offset;
  result *= fieldPositions[index].multiplier;
  result /= fieldPositions[index].divisor;
 
  return result;
}

// print frame on screen and add the interpreted value, as specified by database index number
boolean printInterpretedFrame (CAN_FRAME *frame, int index) {
  long result;

  if  (fieldPositions[index].filterMask != 0 && fieldPositions[index].filterValue != (frame->data.low & fieldPositions[index].filterMask)) {
    return false;
  }
  if (fieldPositions[index].line != 0) {
    printFrame (frame, fieldPositions[index].line, fieldPositions[index].row); // print the raw data
  } else {
    printFrame (frame, (index % screenBlock1) + 1, (index / screenBlock1) * 53 + 1); // print the raw data
  }
  result = getVal (frame, index);
  if (fieldPositions[index].decimals == 0) {
    snprintf(buf, PBUFSIZE, fieldPositions[index].format, result);
  } else {
    if (result >= 0) {
      snprintf(buf, PBUFSIZE, fieldPositions[index].format, result / fieldPositions[index].decimals, result % fieldPositions[index].decimals);
    } else {
      snprintf(buf, PBUFSIZE, fieldPositions[index].format, result / fieldPositions[index].decimals, -result % fieldPositions[index].decimals);
    }
  }
  toConsole (buf);
  return true;
}

// find matching database entries for the frame and print those.
boolean printFrameValues (CAN_FRAME *frame) {
  boolean found = false;
  int i;

  i = idIndex[frame->id];
  if (i >= 0x800) {
    i -= 0x800;
    while (fieldPositions[i].id == frame->id) {
      printInterpretedFrame (frame, i++);
    }
    return (true);
  }
  return false;
}

// print an unknown frame type and reserve a screen position for it
boolean printFrameUnknown (CAN_FRAME *frame) {
  if (frame->id > 0xfff) {                  // overrun or Db entry
    return false;
  } else if (idIndex[frame->id] == 0) {                // open a new position
    idIndex[frame->id] = lastidIndexCol++;
  } else if (idIndex[frame->id] == 0xffff) {        // ignore the frames we don't want
    return true;
  }
  printFrame (frame, ((idIndex[frame->id]-1) % screenBlock2) + screenBlock1 + 2, ((idIndex[frame->id]-1) / screenBlock2) * 34 + 1);
}

// handle one frame
void printFrameAll (CAN_FRAME *frame) {
  if (!printFrameValues (frame))
  printFrameUnknown (frame);
}

//********************************************************************************************************

sendFrame (CAN_FRAME *frame) {
    char buffer[20];
    int bptr = 0;
    int i;

    mode |= 0x100;
    buffer [bptr++] = 0xf1;
    buffer [bptr++] = 0;
    buffer [bptr++] = (frame->id      ) & 0xff;
    buffer [bptr++] = (frame->id >> 8 ) & 0xff;
    buffer [bptr++] = (frame->id >> 16) & 0xff;
    buffer [bptr++] = (frame->id >> 24) & 0xff;
    buffer [bptr++] = 0x0;
    buffer [bptr++] = frame->length;
    for (i = 0; i < frame->length; i++)
        buffer [bptr++] = frame->data.byte[i];
    buffer [bptr++] = 0; // checksum
    write (fd, buffer, bptr);
}

sendFrameData (uint32_t id, uint8_t length, uint32_t data0, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7) {
  CAN_FRAME myFrame;
  myFrame.id = id;
  myFrame.length = length;
  myFrame.data.byte[0] = data0;
  myFrame.data.byte[1] = data1;
  myFrame.data.byte[2] = data2;
  myFrame.data.byte[3] = data3;
  myFrame.data.byte[4] = data4;
  myFrame.data.byte[5] = data5;
  myFrame.data.byte[6] = data6;
  myFrame.data.byte[7] = data7;
  sendFrame (&myFrame);

}

sendIsoTpFlowControl (uint32_t id) {
  CAN_FRAME myFrame;
  usleep (0x20 * 1000);
  sendFrameData (id,    8, 0x30, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00);
}

void sendTestFrameLbcCha () {
  sendFrameData (0x79b, 8, 0x02, 0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);
  sendIsoTpFlowControl (0x79b);
}

void sendTestFrameLbcTemp () {
  sendFrameData (0x79b, 8, 0x02, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00);
  sendIsoTpFlowControl (0x79b);
}

void sendTestFrameLbcShunt () {
  sendFrameData (0x79b, 8, 0x02, 0x21, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00);
  sendIsoTpFlowControl (0x79b);
}

void sendTestFrameEvcMaxPwr () {
  sendFrameData (0x7e4, 8, 0x03, 0x22, 0x34, 0x44, 0x00, 0x00, 0x00, 0x00);
}

void sendTestFrameEvcSoc () {
  sendFrameData (0x7e4, 8, 0x03, 0x22, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00);
}

void sendTestFrameEvcAmp () {
  sendFrameData (0x7e4, 8, 0x03, 0x22, 0x32, 0x04, 0x00, 0x00, 0x00, 0x00);
}

void sendTestFrameEvcVolt () {
  sendFrameData (0x7e4, 8, 0x03, 0x22, 0x32, 0x03, 0x00, 0x00, 0x00, 0x00);
}

void sendWakeupInstrPanel () {
  sendFrameData (0x743, 8, 0x02, 0x10, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00);
}

void sendWakeupUdp () {
  sendFrameData (0x74d, 8, 0x02, 0x10, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00);
}

void *sendRequests (void *voidPtr) {

  int loop = 0;

  while (ctrlC == 0) {

    if (mode & 0x020) {
      if (loop == 5) {
         sendTestFrameLbcCha ();
      } else if (loop == 10) {
         sendTestFrameLbcTemp ();
      }
    }

    if (mode & 0x010) {
      if (loop == 15) {
        sendTestFrameEvcMaxPwr ();
      } else if (loop == 20) {
        sendTestFrameEvcSoc ();
      } else if (loop == 25) {
        sendTestFrameEvcAmp ();
      } else if (loop == 30) {
        sendTestFrameEvcVolt ();
      }
    }

    if (mode & 0x040) {
       sendWakeupInstrPanel ();
       mode &= 0xfbf;
    }

    if (mode & 0x080) {
       sendWakeupUdp ();
       mode &= 0xf7f;
    }

    usleep (100 * 1000);
    if (++loop == 50) loop = 0;
  }
}

//********************************************************************************************************

void gotFrame(CAN_FRAME *frame) {
  ledToggle();
  switch (mode & 0x00f) {
  case 0:
    logFrame (frame);
    break;
  case 1:
    screenToggle();
    printFrameAll (frame);
    break;
  case 2:
    logFrameGVRET (frame);
    break;
  case 3:
    logFrameCRTD (frame);
    break;
  otherwsie:
    screenToggle();
    break;    
  }
  frameCount++;
}

// wrapper so we can redirect to where we want.
void toConsole (char *buf) {
  //Serial.print (buf);
  fputs (buf, stdout);
  fflush(stdout);
}

void clearScreen(){
  snprintf(buf, PBUFSIZE, "%c[2J", 27);
  toConsole (buf);
  screenToggle();
  initIndex();
}

//********************************************************************************************************

void procRXChar(unsigned char c)
{
  CAN_FRAME fr;
  int dataidx;

  switch (rx_state)
  {
  case IDLE:
    if (c == 0xF1) rx_state = GET_COMMAND;
      break;
  case GET_COMMAND:
    switch (c) {
    case 0: //receiving a can frame
      rx_state = BUILD_CAN_FRAME;
      rx_step = 0;
      break;
    default:
      break;
    }
    break;

  case BUILD_CAN_FRAME:
    switch (rx_step) {
    case 0:
      timestamp = c;
      break;
    case 1:
      timestamp |= (uint)(c << 8);
      break;
    case 2:
      timestamp |= (uint)c << 16;
      break;
    case 3:
      timestamp |= (uint)c << 24;
      break;
    case 4:
      ID = c;
      break;
    case 5:
      ID |= c << 8;
      break;
    case 6:
      ID |= c << 16;
      break;
    case 7:
      ID |= c << 24;
      break;
    case 8:
      len = c & 0xF;
      break;
    default:
      if (rx_step < len + 9) {
        data[rx_step - 9] = c;
      } else {
        fr.id = ID;
        fr.length = len;
        for (dataidx = 0; dataidx < fr.length; dataidx++) fr.data.byte[dataidx] = data[dataidx];
        for (           ; dataidx < 8; dataidx++) fr.data.byte[dataidx] = 0;
        gotFrame (&fr);
        rx_state = IDLE;
        rx_step = 0;
      }
      break;
    }
    rx_step++;
    break;

  default:
    break;

  }
}

//********************************************************************************************************

void intHandler(int dummy) {
    ctrlC = -1;
}

// main
int main(int argc, char *argv[])
{
  pthread_t sendRequestsThread;     /* this variable is our reference to the second thread */
  char buffer[1024];
  int bufferPtr;
  int bytesInBuffer;

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd == -1) {
    /* Could not open the port. */
    perror("open_port: Unable to open port arg1 - ");
    exit (1);
  }

  clearScreen();

  signal(SIGINT, intHandler);

  // initialize GVRET
  bufferPtr = 0;
  buffer [bufferPtr++]  = 0xe7; // binary mode
  buffer [bufferPtr++]  = 0xe7;
  write (fd, buffer, bufferPtr); // send to GVRET

  bufferPtr = 0;
  buffer [bufferPtr++]  = 0xf1; // single wire mode
  buffer [bufferPtr++]  = 8;
  buffer [bufferPtr++]  = 0xff;
  write (fd, buffer, bufferPtr); // send to GVRET

  bufferPtr = 0;
  buffer [bufferPtr++]  = 0xf1; // set can0 to 500kps
  buffer [bufferPtr++]  = 5;
  buffer [bufferPtr++]  = 0x20;
  buffer [bufferPtr++]  = 0xa1;
  buffer [bufferPtr++]  = 0x07;
  buffer [bufferPtr++] = 0x0;
  buffer [bufferPtr++] = 0x0;
  buffer [bufferPtr++] = 0x0;
  buffer [bufferPtr++] = 0x0;
  buffer [bufferPtr++] = 0x0;
  buffer [bufferPtr++] = 0x0;
  write (fd, buffer, bufferPtr); // send to GVRET

  bufferPtr = 0;
  buffer [bufferPtr++]  = 0xf1; // set all LED pins to 13
  buffer [bufferPtr++]  = 10;
  buffer [bufferPtr++]  = 13;
  buffer [bufferPtr++]  = 13;
  buffer [bufferPtr++]  = 13;
  write (fd, buffer, bufferPtr); // send to GVRET

  // start reading traffic from GVRET, non blocking
  bytesInBuffer = read(fd, buffer, sizeof(buffer)-2);
  if (bytesInBuffer < 0) {
    fputs("read failed!\n", stderr);
    exit (1);
  }

  /* create a second thread which executes sendRequests() */
  if (pthread_create (&sendRequestsThread, NULL, sendRequests, NULL)) {
    perror("Error creating thread");
    exit (1);
  }

  while (bytesInBuffer >= 0 && ctrlC == 0) {

    if (bytesInBuffer > 0) {
      for (bufferPtr = 0; bufferPtr < bytesInBuffer; bufferPtr++) {
        procRXChar (buffer[bufferPtr]);
      }
    }
    bytesInBuffer = read(fd, buffer, sizeof(buffer)-2);

    char inChar = getch();

    switch (inChar){
    case '0':
      clearScreen();
      mode = (mode & 0xff0);
      break;
    case '1':
      clearScreen();
      mode = (mode & 0xff0) + 1;
      break;
    case '2':
      clearScreen();
      mode = (mode & 0xff0) + 2;
      break;
    case '3':
      clearScreen();
      mode = (mode & 0xff0) + 3;
      break;
    case 'c':
      clearScreen();
      break;
    case 't':
      mode ^= 0x020;
      break;
    case 'v': // wakeup UCP
      mode |= 0x080;
      break;
    case 'w': // wakup Instrument panel
      mode |= 0x040;
      break;
    case 'y':
      mode ^= 0x010;
      break;
    case 'x':
      ctrlC = 1;
      break;
    }

    if (mode & 0x100) {
      screenToggle();
      mode &= 0xeff;
    }

  }

  if  (pthread_join (sendRequestsThread, NULL)) {
    perror("Error joining thread");
  }

  fputs("\nCtrl-C\n", stderr);
  close (fd);
  return 0;
}

