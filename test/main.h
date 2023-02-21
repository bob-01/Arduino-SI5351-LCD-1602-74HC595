#ifndef MAIN_H
#define MAIN_H

//#define ATMEGA328
#define STM32F0

#define VERSION   "1.01a"
//#define SWR

#define Smeter         A0

#ifdef SWR
  #define analogInputFWD    A2
  #define analogInputREF    A1
  #define BUFER_MAX_5ms               50        // Buffer size for 100ms max measurement
  #define PEP_BUFFER                  10 // Buffer size for 1s PEP measurement
  #define BUF_100ms                   20  // Buffer size for 100ms PK measurement

  const	double aref = 4.98;	// (Normally an External 4.96V reference)

  double       AttPwrMeter =         0.0;
  uint16_t     fwd_raw;                        // AD input - 10 bit value, v-forward
  uint16_t     ref_raw;                        // AD input - 10 bit value, v-reverse
  double       Vfwd, Vref;

  double       fwd_dbm;
  double       ref_dbm;
  double       v_fwd, v_ref;         // Calculated voltage
  double		   swr = 1.0;					   // SWR as an absolute value

  double       calibrateFRW =        0.0;
  double       calibrateREF =        0.0;

  double       fwd_power_100us, ref_power_100us, fwd_power_5ms;

  uint32_t     b, _100us_count, _1s_count;                              // PEP ring buffer counter
  static uint32_t db_buff_max_5ms[BUFER_MAX_5ms];
  static uint32_t db_buff_100ms[BUF_100ms];                  // dB voltage information in a one second window
  static uint32_t db_buff_1s[PEP_BUFFER];                    // dB voltage information in a one second window

  uint32_t     power_pep_1s;                   // Calculated PEP power in dBm
  double       power_pk_100ms;                 // Calculated 100ms peak power in dBm

  double       modulation_index =    0.0;

  int32_t      ATT_PWR_METER =       0;
  int32_t      FWR_ERROR =           0;
  int32_t      REF_ERROR =           0;
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

unsigned long _100usTime, _5msTime, _100msTime, _1000msTime;
int32_t IF, IF2, Fmin, Fmax, Ftx = 3100000, Frit = Ftx, STEP;
int32_t SI5351_FXTAL;

boolean resetFlag, enc_block=false, enc_flag=false, rit_flag=false, Button_flag=false, tx_flag=false, vcxo_flag=false, step_flag=false, setup_flag=false;
uint8_t IF_WIDTH_FLAG = false;

uint8_t Enc_state, Enc_last, step_count=3, setup_count=8, xF=1, SI5351_DRIVE_CLK0, SI5351_DRIVE_CLK1, SI5351_DRIVE_CLK2;
uint8_t ftone_flag=false;
int8_t enc_move = 0, mode = 1;
int8_t ENC_SPIN = 1;

uint16_t Ftone, uSMETER, smeter_cnt;

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

#ifdef SWR
void adc_poll(uint8_t analogInputFWD, uint8_t analogInputREF) {
  fwd_raw = analogRead(analogInputFWD);
  ref_raw = analogRead(analogInputREF);
}

void determine_dBm(void) {
  //uint16_t temp;

  //if (fwd_raw < ref_raw) {
  //  temp = fwd_raw;
  //  fwd_raw = ref_raw;
  //  ref_raw = temp;
  //}

#ifdef ANALOG_DETECTOR
  Vfwd = (fwd_raw * aref) / 1024.0;
  // Vp-p / 2 * 0.707 RMS
  Vfwd = (Vfwd / 2) * 0.707;
  Vref = (ref_raw * aref) / 1024.0;
  // Vp-p / 2 * 0.707 RMS
  Vref = (Vref / 2) * 0.707;

  // P(dBm) = 10 log10(v2/(R*p0))
  //dbm = 10 * log10((rms * rms) / 50) + 30 - ref; //from rmsV to dBm at 50R

  fwd_dbm = AttPwrMeter + (10 * log10((Vfwd * Vfwd) / 50) + 30) + calibrateFRW;
  ref_dbm = AttPwrMeter + (10 * log10((Vref * Vref) / 50) + 30) + calibrateREF;
#else
  Vfwd = (fwd_raw * aref) / 1024.0;
  Vref = (ref_raw * aref) / 1024.0;

  fwd_dbm = Vfwd / 0.025;
  fwd_dbm = -75.0 + AttPwrMeter + fwd_dbm + calibrateFRW;

  ref_dbm = Vref / 0.025;
  ref_dbm = -75.0 + AttPwrMeter + ref_dbm + calibrateREF;
#endif
}

double calculate_SWR(double v_fwd, double v_rev) {
  return (1+(v_rev/v_fwd))/(1-(v_rev/v_fwd));
}

void calculate_pk_pep_avg(double v_fwd) {
//  double v_max = 0, v_avg = 0;

  // Find peaks and averages
  db_buff_100ms[b] = 100 * fwd_power_5ms;
  b++;
  if (b >= BUF_100ms) b = 0;

  // Find Peak value within a 100ms sliding window
  uint32_t pk = 0;
  for (uint8_t x = 0; x < BUF_100ms; x++) {
    if (pk < db_buff_100ms[x]) pk = db_buff_100ms[x];
  }

  power_pk_100ms = pk / 100.0;

  // Retrieve Max Value within a 1 second sliding window
//  uint16_t vmax = 0;
//  for (uint16_t x = 0; x < PEP_BUFFER; x++) {
//    v_avg = (v_avg + voltage_buff_1s[x]) / 2;
//    if (vmax < voltage_buff_1s[x]) vmax = voltage_buff_1s[x];
//  }

//  v_max = vmax / 100.0;

	// Amplitude Modulation index
//	modulation_index = abs(100.0 * (v_max-v_avg)/v_avg);
}

/*
  dBm = 10 * log(P)              where P is power in milliwatts; or
  dBm = 10 * log(P) + 30         where P is power in watts

  fwd_power_dbm_5ms = 10 * log10(fwd_power_mw_5ms);
  v_fwd = pow(10, fwd_dbm/20.0);
*/
void determine_power_and_swr(void) {
}
#endif

#endif
