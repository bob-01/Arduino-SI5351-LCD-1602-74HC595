#ifndef MAIN_H
#define MAIN_H

#define VERSION   "1.01a"
#define SWR

#define Smeter         A0

#ifdef SWR
  // Macros
  #ifndef SQR
    #define SQR(x) ((x)*(x))
  #endif

  #ifndef ABS
    #define ABS(x) ((x>0)?(x):(-x))
  #endif

  #define analogInputFWD    A1
  #define analogInputREF    A2
  const	float aref = 4.98;	// (Normally an External 4.96V reference)

  double      att = 56;

  uint16_t     fwd_raw;                        // AD input - 10 bit value, v-forward
  uint16_t     ref_raw;                        // AD input - 10 bit value, v-reverse

  double      fwd_dbm;
  double      ref_dbm;
  double		  swr = 1.0;					// SWR as an absolute value

  double      calibrateFRW = 0.0;
  double      calibrateREF = -2.7;

  double      fwd_power_mw_5ms, ref_power_mw_5ms, fwd_power_5ms, ref_power_5ms;
  double      power_mw_5ms, power_5ms;

  double      power_db_pep;               // Calculated PEP power in dBm
  double      power_db_pk;                // Calculated 100ms peak power in dBm
  double      power_mw_pep;               // Calculated PEP power in mW
  double      power_mw_pk;                // Calculated 100ms peak power in mW

  double      modulation_index = 0.0;

  //----------------
  #define SAMPLE_TIME               5 // Time between samples, milliseconds
  #define PEP_BUFFER              200 // PEP Buffer size, can hold up to 5 second PEP
  #define BUF_SHORT                20 // Buffer size for 100ms Peak
  #define AVG_BUFSHORT             20 // Buffer size for 100ms Average measurement
  #define AVG_BUF1S               200 // Buffer size for 1s Average measurement

  uint16_t PEP_period = 1000/SAMPLE_TIME;
#endif

#define BUTTON_TONE    A3

//#define SWAP_ROTARY             1 // Swap rotary direction
//#define BUTTON_TONE_ENABLE        // A2 Button tone

#define ROT_A     2       //PD2
#define ROT_B     3       //PD3

#ifdef SWAP_ROTARY
#undef ROT_A
#undef ROT_B
#define ROT_A   3
#define ROT_B   2
#endif

#define BUTTON_STEP    7 
#define BUTTON_VCXO    8
#define BUTTON_TX      9 
#define BUTTON_RIT     10
#define TX_OUT         11
#define tone_pin       12
#define IF_WIDTH_PIN   13

#include "si5351.h"
#include "Wire.h"
#include "ShiftedLCD.h"
#include <EEPROM.h> // Для работы со встроенной памятью ATmega

Si5351 si5351;

unsigned long _5msTime, _100msTime, _1000msTime;
unsigned long IF, IF2, Fmin, Fmax, Ftx = 3100000, Frit = Ftx, STEP;
unsigned long SI5351_FXTAL;

boolean resetFlag, enc_block=false, enc_flag=false, rit_flag=false, Button_flag=false, tx_flag=false, vcxo_flag=false, step_flag=false, setup_flag=false;
uint8_t IF_WIDTH_FLAG = false;

uint8_t Enc_state, Enc_last, step_count=3, menu_count=0, setup_count=8, xF=1, SI5351_DRIVE_CLK0, SI5351_DRIVE_CLK1, SI5351_DRIVE_CLK2;
uint8_t ftone_flag=false;
int8_t enc_move = 0, mode = 1;
int8_t ENC_SPIN = 1;

uint16_t Ftone, uSMETER;

boolean BUTTON_TONE_FLAG = false;

// button-left, button-right and button-encoder; single-click, double-click, push-long, push-and-turn
enum event_t { BL=0x10, BR=0x20, BE=0x30, BT= 0x40, SC=0x01, DC=0x02, PL=0x04, PLC=0x05, PT=0x0C };
volatile uint8_t event;

bool curs = false;

uint16_t AdcReadAvrValue(uint8_t pin) {
  uint16_t val[14], tmp;
  uint8_t i, j, count;

  count = (sizeof(val)/sizeof(int));

  for (i = 0; i < count; i++) {
    val[i] = analogRead(pin);
  }

  for (i = 0; i < count; i++) {
  for (j = 0; j < count - i - 1; j++) {
    if (val[j] > val[j+1]) {
      tmp = val[j];
      val[j] = val[j+1];
      val[j+1] = tmp;
    }
  }}

  return val[(int)(count/2)];
}

void adc_poll(uint8_t analogInputFWD, uint8_t analogInputREF) {
  uint16_t tmp;

  fwd_raw = analogRead(analogInputFWD);
  tmp = analogRead(analogInputFWD);
  fwd_raw = (int)((fwd_raw + tmp)/2);

  ref_raw = analogRead(analogInputREF);
  tmp = analogRead(analogInputREF);
  ref_raw = (int)((ref_raw + tmp)/2);
}

void pswr_determine_dBm(void) {
  uint16_t temp;
  double Vfwd, Vref, tmp_dbm;

  if (fwd_raw < ref_raw) {
    temp = fwd_raw;
    fwd_raw = ref_raw;
    ref_raw = temp;
  }

  Vfwd = (fwd_raw * aref) / 1024.0;
  Vref = (ref_raw * aref) / 1024.0;

  tmp_dbm = Vfwd / 0.025;
  tmp_dbm = -att + tmp_dbm + calibrateFRW;
  fwd_dbm = (tmp_dbm + fwd_dbm)/2;

  tmp_dbm = Vref / 0.025;
  tmp_dbm = -att + tmp_dbm + calibrateREF;
  ref_dbm = (tmp_dbm + ref_dbm)/2;
}

double calculate_SWR(double v_fwd, double v_rev) {
  return (1+(v_rev/v_fwd))/(1-(v_rev/v_fwd));
}

void calculate_pk_pep_avg(void) {
  // For measurement of peak and average power
  static int16_t db_buff[PEP_BUFFER];           // dB voltage information in a one second window
  static int16_t db_buff_short[BUF_SHORT];      // dB voltage information in a 100 ms window
  static double  p_avg_buf[AVG_BUFSHORT];       // a short buffer of all instantaneous power measurements, for short average
  static double  p_avg_buf1s[AVG_BUF1S];        // a one second buffer of instantanous power measurements, for 1s average
  static double  p_1splus;                      // averaging: all power measurements within a 1s window added together
  static double  p_plus;                        // averaging: all power measurements within a shorter window added together
  static uint16_t a;                            // PEP ring buffer counter
  static uint16_t b;                            // 100ms ring buffer counter
  static uint16_t d;                            // avg: short ring buffer counter
  static uint16_t e;                            // avg: 1s ring buffer counter
  int16_t max = -32767, pk = -32767;                // Keep track of Max (1s) and Peak (100ms) dB voltage

  // Find peaks and averages
  // Multiply by 100 to make suitable for integer value
  // Store dB value in two ring buffers
  db_buff[a] = db_buff_short[b] = 100 * power_5ms;
  // Advance PEP (1, 2.5 or 5s) and 100ms ringbuffer counters
  a++, b++;
  if (a >= PEP_period) a = 0;
  if (b == BUF_SHORT) b = 0;

  // Retrieve Max Value within a 1 second sliding window
  for (uint16_t x = 0; x < PEP_period; x++) {
    if (max < db_buff[x]) max = db_buff[x];
  }

  // Find Peak value within a 100ms sliding window
  for (uint8_t x = 0; x < BUF_SHORT; x++) {
    if (pk < db_buff_short[x]) pk = db_buff_short[x];
  }

  // PEP
  power_db_pep = max / 100.0;
  power_mw_pep = pow(10,power_db_pep/10.0);

  // Peak (100 milliseconds)
  power_db_pk = pk / 100.0;
  power_mw_pk = pow(10,power_db_pk/10.0);
/*
  //------------------------------------------
  // Average power buffers, short period (100 ms)
  p_avg_buf[d++] = power_mw;                    // Add the newest value onto ring buffer
  p_plus = p_plus + power_mw;                   // Add latest value to the total sum of all measurements in [100ms]
  if (d == AVG_BUFSHORT) d = 0;                 // wrap around
  p_plus = p_plus - p_avg_buf[d];               // and subtract the oldest value in the ring buffer from the total sum
  power_mw_avg = p_plus / (AVG_BUFSHORT-1);     // And finally, find the short period average

  //------------------------------------------
  // Average power buffers, 1 second
  p_avg_buf1s[e++] = power_mw;                  // Add the newest value onto ring buffer
  p_1splus = p_1splus + power_mw;               // Add latest value to the total sum of all measurements in 1s
  if (e == AVG_BUF1S) e = 0;                    // wrap around
  p_1splus = p_1splus - p_avg_buf1s[e];         // and subtract the oldest value in the ring buffer from the total sum
  power_mw_1savg = p_1splus / (AVG_BUF1S-1);    // And finally, find the one second period average
  
  //power_db_avg = 10 * log10(power_mw_avg);

  //// Modulation index in a 1s window
  //double v1, v2;
  //v1 = sqrt(ABS(power_mw_pep));
  //v2 = sqrt(ABS(power_mw_avg1s));
  //modulation_index = (v1-v2) / v2;
*/
}

void determine_power_and_swr(void) {
  double f_inst;                                // Calculated Forward voltage
  double r_inst;                                // Calculated Reverse voltage
  double tmp;

  // Instantaneous forward voltage and power, milliwatts and dBm
  f_inst = pow(10, fwd_dbm/20.0);		// (We use voltage later on, for SWR calc)
  fwd_power_mw_5ms = SQR(f_inst);			      // P_mw = (V*V) (current and resistance have already been factored in)
  fwd_power_5ms = 10 * log10(fwd_power_mw_5ms);

  // Instantaneous reverse voltage and power
  r_inst = pow(10,(ref_dbm)/20.0);
  ref_power_mw_5ms = SQR(r_inst);
  ref_power_5ms = 10 * log10(ref_power_mw_5ms);

  // Instantaneous Real Power Output
  power_mw_5ms = fwd_power_mw_5ms - ref_power_mw_5ms;
  power_5ms = 10 * log10(power_mw_5ms);

  calculate_pk_pep_avg();                       // Determine Peak, PEP and AVG
  tmp = calculate_SWR(f_inst, r_inst);                // and determine SWR
  swr = (tmp + swr)/2;
}
#endif
