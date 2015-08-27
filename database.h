//********************************************************************************************************

// the database!!!

typedef struct
{
  unsigned id;			// CAN id
  uint32_t filterMask;		// mask
  uint32_t filterValue;		// value
  unsigned startbit;
  unsigned endbit;
  long divisor;
  long multiplier;
  long offset;
  int decimals;			// decimals divisor. 0 for integer
  char *format;			// printf style. a %ld or two %ld's when a decimals is specified
  int line;			// 0=auto position on screen
  int row;
} fieldPosition;

const fieldPosition fieldPositions [] = {
  {0x0c6, 0,     0,     0, 15,   1,   1, 0x8000,   0, "Steering pos:%5ld",         0,   0},
  {0x0c6, 0,     0,    16, 29,   1,   1, 0x2000,   0, "Steering ac: %5ld",         0,   0},
  {0x17e, 0,     0,    50, 51,   1,   1,      0,   0, "Gear:         %4ld",        0,   0},
  {0x12e, 0,     0,     0,  7,   1,   1,    198,   0, "Accel F/R:    %4ld",        0,   0},
  {0x12e, 0,     0,     8, 23,   1,   1, 0x8000,   0, "Incl:         %4ld",        0,   0},
  {0x12e, 0,     0,    24, 35,   1,   1,  0x800,   0, "Accel L/R:    %4ld",        0,   0},
  {0x186, 0,     0,     0, 11,   4,   1,      0,  10, "Speed(c):   %3ld.%02ld",    0,   0},
  {0x186, 0,     0,    16, 27,   1,   1,      0,   0, "Accel(a):     %4ld",        0,   0},
  {0x186, 0,     0,    28, 39,   1,   1,      0,   0, "Accel(b):     %4ld",        0,   0},
  {0x186, 0,     0,    40, 49,   1,   1,      0,   0, "Pedal:        %4ld",        0,   0},
  {0x18a, 0,     0,    16, 25,   1,   1,      0,   0, "Pedal cc:     %4ld",        0,   0},
  {0x18a, 0,     0,    31, 38,   1,   1,      0,   0, "Regen?:       %4ld",        0,   0},
  {0x1f6, 0,     0,    20, 20,   1,   5,      0,  10, "Break pedal:  %4ld",        0,   0},
  {0x1fd, 0,     0,     0,  7,   1,   5,      0,  10, "Amp 12V:      %2ld.%01ld",  0,   0},
  {0x1fd, 0,     0,    48, 55,   1,   1,   0x50,   0, "KwDash:       %4ld",        0,   0},
  {0x29a, 0,     0,     0, 15,   1,   1,      0,   0, "Speed FR:    %5ld",         0,   0},
  {0x29a, 0,     0,    16, 31,   1,   1,      0,   0, "Speed FL:    %5ld",         0,   0},
  {0x29a, 0,     0,    32, 47,   1,   1,      0, 100, "Speed F100: %3ld.%02ld",    0,   0},
  {0x29c, 0,     0,     0, 15,   1,   1,      0,   0, "Speed RR:    %5ld"     ,    0,   0},
  {0x29c, 0,     0,    16, 31,   1,   1,      0,   0, "Speed RL:    %5ld",         0,   0},
  {0x29c, 0,     0,    48, 63,   1,   1,      0, 100, "Speed R100: %3ld.%02ld",    0,   0},
  {0x352, 0,     0,    24, 31,   1,   1,      0,   0, "Break force  %5ld",         0,   0},
  {0x35c, 0,     0,     4, 15,   1,   1,      0,   0, "Key-Start:   %5ld",         0,   0},
  {0x35c, 0,     0,    16, 39,   1,   1,      0,   0, "Minutes:   %7ld",           0,   0}, // minutes since initial power up of the car, i.e. age
  {0x3f7, 0,     0,     2,  3,   1,   1,      0,   0, "Gear??:       %4ld",        0,   0}, // Zoe 
  {0x42a, 0,     0,    16, 23,   1,   1,      0,   0, "Temp set:    %5ld",         0,   0}, // Fluence. Zoe has this ID, but not this interpretation
  {0x42e, 0,     0,     0, 12,  49,   1,      0,   0, "SOC(a):      %5ld",         0,   0}, // Divisor 48 for Fluence, or maybe it is a normalisation factor
  {0x42e, 0,     0,    24, 35,  16, 100,      0, 100, "AC V?:      %3ld.%02ld",    0,   0},
  {0x42e, 0,     0,    38, 43,   1,   1,      0,   1, "AC amps:       %3ld",       0,   0}, // 0x0f for 3 phases
  {0x42e, 0,     0,    56, 63,   1,   1,      0,  10, "EOL kWh:     %3ld.%01ld",   0,   0},
  {0x4f8, 0,     0,     0,  1,   1,  -1,     -2,   0, "Start:        %4ld",        0,   0},
  {0x4f8, 0,     0,     4,  5,   1,  -1,     -2,   0, "Park.break:   %4ld",        0,   0},
  {0x534, 0,     0,    32, 40,   1,   1,     40,   0, "Temp out:     %4ld",        0,   0}, // Fluence. Not found in Zoe yet
  {0x5d7, 0,     0,     0, 15,   1,   1,      0, 100, "Speed(a):   %3ld.%02ld",    0,   0},
  {0x5d7, 0,     0,    16, 43,   1,   1,      0, 100, "Odo:      %5ld.%02ld",      0,   0},
  {0x5de, 0,     0,     1,  1,   1,   1,      0,   0, "Right:       %5ld",         0,   0},
  {0x5de, 0,     0,     2,  2,   1,   1,      0,   0, "Left:        %5ld",         0,   0},
  {0x5de, 0,     0,     5,  5,   1,   1,      0,   0, "Park light:  %5ld",         0,   0},
  {0x5de, 0,     0,     6,  6,   1,   1,      0,   0, "Head light:  %5ld",         0,   0},
  {0x5de, 0,     0,     7,  7,   1,   1,      0,   0, "Beam light:  %5ld",         0,   0},
  {0x5de, 0,     0,    12, 12,   1,   1,      0,   0, "FL door open:    %1ld",     0,   0},
  {0x5de, 0,     0,    14, 14,   1,   1,      0,   0, "FR door open:    %1ld",     0,   0},
  {0x5de, 0,     0,    17, 17,   1,   1,      0,   0, "RL door open:    %1ld",     0,   0},
  {0x5de, 0,     0,    19, 19,   1,   1,      0,   0, "RR door open:    %1ld",     0,   0},
  {0x5de, 0,     0,    59, 59,   1,   1,      0,   0, "Hatch door open: %1ld",     0,   0},
  {0x5ee, 0,     0,     0,  4,   1,   1,      0,   0, "Headlights    %4ld",        0,   0}, // fesch report differently, all lights in this nibble
  {0x5ee, 0,     0,    16, 19,   1,   1,      0,   0, "Door locks    %4ld",        0,   0},
  {0x5ee, 0,     0,    20, 24,   1,   1,      0,   0, "Flashers      %4ld",        0,   0},
  {0x5ee, 0,     0,    24, 27,   1,   1,      0,   0, "Doors         %4ld",        0,   0},
  {0x646, 0,     0,     8, 15,   1,   1,      0,  10, "avg trB cons  %2ld.%01ld",  0,   0},
  {0x646, 0,     0,    16, 32,   1,   1,      0,   0, "trB dist     %5ld",         0,   0},
  {0x646, 0,     0,    33, 47,   1,   1,      0,  10, "trB cons   %5ld.%01ld",     0,   0},
  {0x646, 0,     0,    48, 59,   1,   1,      0,  10, "trB spd    %5ld.%01ld",     0,   0},
  {0x653, 0,     0,     9,  9,   1,   1,      0,   0, "dr seatbelt   %4ld",        0,   0},
  {0x654, 0,     0,    24, 31,   1,   1,      0,   0, "SOC(b):       %4ld",        0,   0},
  {0x654, 0,     0,    32, 41,   1,   1,      0,   0, "Time to full  %4ld",        0,   0},
  {0x654, 0,     0,    42, 51,   1,   1,      0,   0, "Km avail:     %4ld",        0,   0},
  {0x654, 0,     0,    52, 61,   1,   1,      0,  10, "kw/100Km      %2ld.%01ld",  0,   0},
  {0x658, 0,     0,     0, 31,   1,   1,      0,   0, "S# batt:%10ld",             0,   0},

  {0x658, 0,     0,    32, 39,   1,   1,      0,   0, "Bat health    %4ld",        0,   0},
  {0x699, 0,     0,     8, 15,   2,   1,      0,   0, "Clima temp    %4ld",        0,   0}, // Clima 0x699 is Zoe specific, so we can reuse line numbers for Fluence
  {0x699, 0,     0,    24, 27,   1,   1,      0,   0, "Clima fan     %4ld",        0,   0}, // 16-19 fan speed 1-8, f=off							
  {0x699, 0,     0,     3,  3,   1,   1,      0,   0, "Clima reardfr %4ld",        0,   0},
  {0x699, 0,     0,     5,  5,   1,   1,      0,   0, "Clima Maxdfr  %4ld",        0,   0},
  {0x699, 0,     0,     6,  6,   1,   1,      0,   0, "Clima autofan %4ld",        0,   0},
  {0x699, 0,     0,     4,  4,   1,  -1,     -1,   0, "Clima auto    %4ld",        0,   0},
  {0x699, 0,     0,    16, 16,   1,   1,      0,   0, "Clima wshld   %4ld",        0,   0},
  {0x699, 0,     0,    18, 18,   1,   1,      0,   0, "Clima face    %4ld",        0,   0},
  {0x699, 0,     0,    19, 19,   1,   1,      0,   0, "Clima feet    %4ld",        0,   0},
  {0x699, 0,     0,    21, 21,   1,   1,      0,   0, "Clima recirc  %4ld",        0,   0},
  {0x699, 0,     0,    28, 31,   1,   1,      0,   0, "Clima chg     %4ld",        0,   0}, // 20-23 disp a=auto,4=FrWinshldHeat,3=readHeat,8=fan,5=airDistr,6=airCirc,b=AC,1=temp
  {0x699, 0,     0,    55, 55,   1,   1,      0,   0, "Clima AC      %4ld",        0,   0},
  {0x66a, 0,     0,     8, 15,   1,   1,      0,   0, "CruisC spd    %4ld",        0,   0},
  {0x66a, 0,     0,    42, 42,   1,   1,      0,   0, "CruisC ?      %4ld",        0,   0},
  {0x66a, 0,     0,     5,  7,   1,   1,      0,   0, "CruisC mode   %4ld",        0,   0}, // 0=off,5=cc,2=limiter
  {0x69f, 0,     0,     0, 31,   1,   1,      0,   0, "S# car: %10ld",             0,   0},
  {0x6f8, 0,     0,    16, 23,  16, 100,      0, 100, "Bat12:       %2ld.%02ld",   0,   0},

// multiplexed
//  {0x7bb, 0x000000ff, 0x00000023, 40, 55,   1,   1,      0, 100, "Max regen fl:%2ld.%02ld",        0,   0},

/*
  {0x7bb, 0x000000ff, 0x00000010, 48, 55,   1,   1,     40,   0, "Cell 01 temp: %4ld",        0,   0},
  {0x7bb, 0x000000ff, 0x00000021, 16, 23,   1,   1,     40,   0, "Cell 02 temp: %4ld",        0,   0}, // mask 2x is an answer array that started in the last 4 bytes of "10". State machine is needed
  {0x7bb, 0x000000ff, 0x00000021, 40, 47,   1,   1,     40,   0, "Cell 03 temp: %4ld",        0,   0}, 
  {0x7bb, 0x000000ff, 0x00000022,  8, 15,   1,   1,     40,   0, "Cell 04 temp: %4ld",        0,   0}, 
  {0x7bb, 0x000000ff, 0x00000022, 32, 39,   1,   1,     40,   0, "Cell 05 temp: %4ld",        0,   0}, 
  {0x7bb, 0x000000ff, 0x00000022, 56, 63,   1,   1,     40,   0, "Cell 06 temp: %4ld",        0,   0}, 
  {0x7bb, 0x000000ff, 0x00000023, 24, 31,   1,   1,     40,   0, "Cell 07 temp: %4ld",        0,   0}, 
  {0x7bb, 0x000000ff, 0x00000023, 48, 55,   1,   1,     40,   0, "Cell 08 temp: %4ld",        0,   0}, 
  {0x7bb, 0x000000ff, 0x00000024, 16, 23,   1,   1,     40,   0, "Cell 09 temp: %4ld",        0,   0}, 
  {0x7bb, 0x000000ff, 0x00000024, 40, 47,   1,   1,     40,   0, "Cell 10 temp: %4ld",        0,   0}, 
  {0x7bb, 0x000000ff, 0x00000025,  8, 15,   1,   1,     40,   0, "Cell 11 temp: %4ld",        0,   0}, 
  {0x7bb, 0x000000ff, 0x00000025, 32, 39,   1,   1,     40,   0, "Cell 12 temp: %4ld",        0,   0}, */
  {0x7ec, 0xffffffff, 0x44346204, 32, 39,   1,  30,      0, 100, "Max ch pwr:  %2ld.%02ld",   0,   0}, // ENDIAN!!!
  {0x7ec, 0xffffffff, 0x02206205, 32, 47,   1,   2,      0, 100, "SOC(c):      %2ld.%02ld",   0,   0}, //
  {0x7ec, 0xffffffff, 0x03326205, 32, 47,   1,  50,      0, 100, "Batt V:     %3ld.%02ld",    0,   0}, //
  {0x7ec, 0xffffffff, 0x04326205, 32, 47,   1,  19, 0x8000, 100, "Batt A:     %3ld.%02ld",    0,   0}, //


// The next 17 lines are put in here to ensure OBD packets are displayed on a prominent place There is not
// interpreteation of the OBD packets, and to be honest, I don't even know if the Zoe does any OBD

//{0x7df, 0,     0,     0, 23,   1,   1,      0,   0, "OBD br: %10ld",             1, 157},//{0x7e0, 0,     0,     0, 23,   1,   1,      0,   0, "OBD t1: %10ld",             2, 157},
//{0x7e1, 0,     0,     0, 23,   1,   1,      0,   0, "OBD t2: %10ld",             3, 157},
//{0x7e2, 0,     0,     0, 23,   1,   1,      0,   0, "OBD t3: %10ld",             4, 157},
//{0x7e3, 0,     0,     0, 23,   1,   1,      0,   0, "OBD t4: %10ld",             5, 157},
//{0x7e4, 0,     0,     0, 23,   1,   1,      0,   0, "OBD t5: %10ld",             6, 157},
//{0x7e5, 0,     0,     0, 23,   1,   1,      0,   0, "OBD t6: %10ld",             7, 157},
//{0x7e6, 0,     0,     0, 23,   1,   1,      0,   0, "OBD t7: %10ld",             8, 157},
//{0x7e7, 0,     0,     0, 23,   1,   1,      0,   0, "OBD t8: %10ld",             9, 157},
//{0x7e8, 0,     0,     0, 23,   1,   1,      0,   0, "OBD r1: %10ld",            10, 157},
//{0x7e9, 0,     0,     0, 23,   1,   1,      0,   0, "OBD r2: %10ld",            11, 157},
//{0x7ea, 0,     0,     0, 23,   1,   1,      0,   0, "OBD r3: %10ld",            12, 157},
//{0x7eb, 0,     0,     0, 23,   1,   1,      0,   0, "OBD r4: %10ld",            13, 157},
//{0x7ec, 0,     0,     0, 23,   1,   1,      0,   0, "OBD r5: %10ld",            14, 157},
//{0x7ed, 0,     0,     0, 23,   1,   1,      0,   0, "OBD r6: %10ld",            15, 157},
//{0x7ee, 0,     0,     0, 23,   1,   1,      0,   0, "OBD r7: %10ld",            16, 157},
//{0x7ef, 0,     0,     0, 23,   1,   1,      0,   0, "ODB r8: %10ld",            17, 157},

  {0x000, 0,     0,     0,  0,   0,   0,      0,   0, NULL,                        0,   0}
};
//{0x42e, 0,     0,    39, 44,   1,   1,      0,   0, "Batt A?:     %5ld",         5, 53}, // seems not for Zoe

const int fixedPositions [] = { // these are ID's we know, but are not interpreted yes. Added here so they are in sequence and we can easily see new ones pop up.
  0x130,
  0x17a,
  0x1f6,
  0x1f8,
  0x212,
  0x218,
  0x242,
  0x2b7,
//  0x352,
  0x354,
  0x391,
  0x3b7,
  0x427,
  0x430,
  0x432,
  0x433,
  0x439,
  0x4fa,
  0x500,
  0x505,
  0x511,
  0x552,
  0x563,
  0x581,
  0x5da,
//  0x5de,
  0x5df,
  0x5ef,
  0x62c,
  0x62d,
  0x634,
  0x637,
  0x638,
//  0x646,
  0x650,
  0x652,
  0x653,
  0x656,
  0x657,
  0x65b,
  0x665,
  0x666,
  0x668,
  0x66d,
  0x671,
  0x68b,
  0x68c,
  0x6fb,
  0x79b,
//0x7bb,
  0
};

