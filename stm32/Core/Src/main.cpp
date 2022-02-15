/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LCD.h"
#include "encoder.h"
#include "si5351.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUTTONS (~GPIOA->IDR & ENC_BUTTON_Pin)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
LCD lcd;

volatile bool change = true;
extern int8_t encoder_val;

int32_t freq = 3100000;

// 0=not in menu, 1=selects menu item, 2=selects parameter value
volatile uint8_t menumode = 0;

uint8_t prev_bandval = 1;
uint8_t bandval = 1;
#define N_BANDS 12
const char* band_label[N_BANDS] = { "160m", "100m", "80m", "60m", "40m", "30m", "20m", "17m", "15m", "12m", "10m", "6m" };

const char* stepsize_label[] = { "10M", "1M", "0.5M", "100k", "10k", "1k", "0.5k", "100", "10", "1" };
enum step_t { STEP_10M, STEP_1M, STEP_500k, STEP_100k, STEP_10k, STEP_1k, STEP_500, STEP_100, STEP_10, STEP_1 };
uint32_t stepsizes[10] = { 10000000, 1000000, 500000, 100000, 10000, 1000, 500, 100, 10, 1 };
volatile uint8_t stepsize = STEP_1k;
uint8_t prev_stepsize[] = { STEP_1k, STEP_500 }; //default stepsize for resp. SSB, CW

volatile uint8_t prev_menumode = 0;
volatile int8_t menu = 0;  // current parameter id selected in menu

volatile uint8_t event;
typedef enum {
   BUTTON_TX = 0x10, BUTTON_IF = 0x20, BUTTON_ENCODER = 0x30, BUTTON_RIT = 0x40
} event_t;

typedef enum
{
    NO_PRESS,
    SINGLE_PRESS = 0x01,
    LONG_PRESS = 0x02,
    DOUBLE_PRESS = 0x03,
    PRESS_TURN = 0x04,
    PLC = 0x05
} eButtonEvent;

typedef enum
{
  _NULL_PARAM, VOLUME, MODE, FILTER, BAND, STEP, VFOSEL, RIT, AGC, NR,
  ATT, ATT2, SMETER, SWRMETER, CWDEC, CWTONE, CWOFF, SEMIQSK, KEY_WPM, KEY_MODE,
  KEY_PIN, KEY_TX, VOX, VOXGAIN, DRIVE, TXDELAY, MOX, CWINTERVAL,
  CWMSG1, CWMSG2, CWMSG3, CWMSG4, CWMSG5, CWMSG6, PWM_MIN, PWM_MAX, SI5351_FXTAL, SR,
  CPULOAD, PARAM_A, PARAM_B, PARAM_C, VERS,
  ALL = 0xff
} eParams_t;

const char* offon_label[2] = {"OFF", "ON"};
const char* smode_label[] = { "OFF", "dBm", "S", "S-bar", "wpm", "Vss" };
const char* swr_label[] = { "OFF", "FWD-SWR", "FWD-REF", "VFWD-VREF" };
const char* att_label[] = { "0dB", "-13dB", "-20dB", "-33dB", "-40dB", "-53dB", "-60dB", "-73dB" };

volatile int8_t volume = 12;
volatile uint8_t filt = 0;

uint8_t eeprom_version;
uint32_t eeprom_addr;

// Support functions for parameter and menu handling
typedef enum { UPDATE, UPDATE_MENU, NEXT_MENU, LOAD, SAVE, SKIP, NEXT_CH } action_t;

const char* mode_label[5] = { "LSB", "USB", "CW ", "FM ", "AM " };
typedef enum { LSB, USB, CW, FM, AM } eMode_t;
volatile uint8_t mode = AM;

#define N_FILT 7
uint8_t prev_filt[] = { 0 , 4 }; // default filter for modes resp. CW, SSB
const char* filt_label[N_FILT + 1] = { "Full", "3000", "2400", "1800", "500", "200", "100", "50" };

static uint8_t pwm_min = 0;    // PWM value for which PA reaches its minimum: 29 when C31 installed;   0 when C31 removed;   0 for biasing BS170 directly
static uint8_t pwm_max = 128;  // PWM value for which PA reaches its maximum:                                              128 for biasing BS170 directly

unsigned long si5351_XTAL = SI5351_XTAL_FREQ;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
void stepsize_showcursor();
void process_encoder_tuning_step(int8_t steps);
void display_vfo(int32_t freq);
void show_banner();
void stepsize_change(int8_t val);
template <typename T> void paramAction(uint8_t action, T& value, uint8_t menuid, const char *label, const char* enumArray[], int32_t _min, int32_t _max, bool continuous);
int8_t paramAction(uint8_t action, uint8_t id);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  lcd.init();
  show_banner();
  lcd.setCursor(7, 0); lcd.print(" R"); lcd.print(VERSION); lcd.blanks();

  si5351_init(&hi2c1, SI5351_BUS_BASE_ADDR, SI5351_CRYSTAL_LOAD_10PF, si5351_XTAL, 0);
  si5351_drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
  si5351_drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);
  si5351_drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);
  
  si5351_set_freq(100 * 1e6 * 100ULL, SI5351_CLK0);
  si5351_set_freq(101 * 1e6 * 100ULL, SI5351_CLK1);
  si5351_set_freq(102 * 1e6 * 100ULL, SI5351_CLK2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (BUTTONS) {
      if(!((event & LONG_PRESS) || (event & PLC))) {  // hack: if there was long-push before, then fast forward
        event = SINGLE_PRESS;

        uint32_t t0 = HAL_GetTick();
        
        for(; BUTTONS;) { // until released or long-press
          if((HAL_GetTick() - t0) > 300) { event = LONG_PRESS; break; }
        }

        //debounce
        HAL_Delay(10);
        // until 2nd press or timeout
        for(; (event != LONG_PRESS) && ((HAL_GetTick() - t0) < 500);) {
          if(BUTTONS) { event = DOUBLE_PRESS; break; }
        }

        // until released, or encoder is turned while longpress
        for(; BUTTONS;) {
          if(encoder_val && event == LONG_PRESS) { event = PRESS_TURN; break; }
        }

        event |= BUTTON_ENCODER;
      } else {
        event = (event & 0xf0) | ((encoder_val) ? PRESS_TURN : PLC);
      }

      switch(event) {
        case BUTTON_ENCODER|SINGLE_PRESS:
          if (!menumode) {
            stepsize_change(+1);
            break;
          }

          // short left-click while in menu: enter value selection screen
          if(menumode == 1) { menumode = 2; break; }
          if(menumode > 1) { menumode = 1; break; }
          break;
        case BUTTON_ENCODER|DOUBLE_PRESS:
          // short left-click while in default screen: enter menu mode
          if(!menumode) { menumode = 1; if(menu == 0) menu = 1; break; }
          // short left-click while in value selection screen: save, and return to default screen
          if(menumode) { menumode = 0; paramAction(SAVE, menu); }
          break;
        case BUTTON_ENCODER|LONG_PRESS: if(!menumode) { stepsize_change(-1); break; }
        case BUTTON_ENCODER|PRESS_TURN:
            for(; BUTTONS;) {
            if(encoder_val) {
              paramAction(UPDATE, VOLUME);
              if(volume < 0) { volume = 10; paramAction(SAVE, VOLUME); }
              paramAction(SAVE, VOLUME);
            }
          }
          change = true; // refresh display
          break;
        }
    } else event = 0;  // no button pressed: reset event

    // Show parameter and value
    if ((menumode) || (prev_menumode != menumode)) {
      int8_t encoder_change = encoder_val;

      if((menumode == 1) && encoder_change) {
        menu += encoder_val;   // Navigate through menu
        menu = max(1 /* 0 */, min(menu, N_PARAMS));
        menu = paramAction(NEXT_MENU, menu);  // auto probe next menu item (gaps may exist)
        encoder_val = 0;
      }

      if(encoder_change || (prev_menumode != menumode)) paramAction(UPDATE_MENU, (menumode) ? menu : 0);  // update param with encoder change and display
      prev_menumode = menumode;

      if(menumode == 2) {
        if(encoder_change) {
        }
      }
    }

    if(menumode == 0) {
      if(encoder_val) {  // process encoder tuning steps
        process_encoder_tuning_step(encoder_val);
        encoder_val = 0;
      }
    }

    if(change) {  // only change if TX is OFF, prevent simultaneous I2C bus access
      change = false;
      if(menumode == 0) {
        display_vfo(freq);
        stepsize_showcursor();
      }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_SYSCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x20303E5D;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_DATA_Pin|LCD_CLK_Pin|LCD_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : ENC_A_Pin ENC_B_Pin */
  GPIO_InitStruct.Pin = ENC_A_Pin|ENC_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ENC_BUTTON_Pin */
  GPIO_InitStruct.Pin = ENC_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(ENC_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_DATA_Pin LCD_CLK_Pin LCD_EN_Pin */
  GPIO_InitStruct.Pin = LCD_DATA_Pin|LCD_CLK_Pin|LCD_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

}

/* USER CODE BEGIN 4 */
void process_encoder_tuning_step(int8_t steps)
{
  int32_t stepval = stepsizes[stepsize];
  freq += steps * stepval;
  freq = max(10000, min(999999999, freq));
  change = true;
}

void stepsize_showcursor() {
  lcd.setCursor(stepsize + 1, 1);  // display stepsize with cursor
  lcd.cursor();
}

void stepsize_change(int8_t val) {
  stepsize += val;
  if(stepsize < STEP_1M) stepsize = STEP_10;
  if(stepsize > STEP_10) stepsize = STEP_1M;
  if(stepsize == STEP_10k || stepsize == STEP_500k) stepsize += val;
  stepsize_showcursor();
}

void show_banner() {
  lcd.setCursor(0, 0);
  lcd.print("uADG");
  lcd.blanks(); lcd.blanks();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if(GPIO_Pin == ENC_A_Pin || GPIO_Pin == ENC_B_Pin) {
    encTick();
  }
}

void display_vfo(int32_t freq) {
  lcd.noCursor();
  lcd.setCursor(0, 1);

  int32_t scale=10e7;
  if(freq/scale == 0) { lcd.print(' '); scale/=10; }
  if(freq/scale == 0) { lcd.print(' '); scale/=10; }

  for(; scale!=1; freq %= scale, scale /= 10) {
    lcd.print(char('0' + freq/scale));
    if(scale == (int32_t)1e3 || scale == (int32_t)1e6) lcd.print('.');
  }
  
  lcd.print(' '); lcd.print(mode_label[mode]); lcd.print(' ');
  lcd.setCursor(15, 1); lcd.print('R');
}

// output menuid in x.y format
void printmenuid(uint8_t menuid){
  static const char seperator[] = {'.', ' '};
  uint8_t ids[] = {(uint8_t)(menuid >> 4), (uint8_t)(menuid & 0xF)};
  for(int i = 0; i < 2; i++){
    uint8_t id = ids[i];
    if(id >= 10){
      id -= 10;
      lcd.print('1');
    }
    lcd.print(char('0' + id));
    lcd.print(seperator[i]);
  }
}

void printlabel(uint8_t action, uint8_t menuid, const char *label){
  if(action == UPDATE_MENU) {
    lcd.setCursor(0, 0);
    printmenuid(menuid);
    lcd.print(label); lcd.blanks(); lcd.blanks();
    lcd.setCursor(0, 1); // value on next line
    if(menumode >= 2) lcd.print('>');
  } else { // UPDATE (not in menu)
    lcd.setCursor(0, 1); lcd.print(label); lcd.print(": ");
  }
}

void actionCommon(uint8_t action, uint8_t *ptr, uint8_t size){
  //uint8_t n;
  switch(action){
    case LOAD:
      //for(n = size; n; --n) *ptr++ = eeprom_read_byte((uint8_t *)eeprom_addr++);
//      eeprom_read_block((void *)ptr, (const void *)eeprom_addr, size);
      break;
    case SAVE:
      //noInterrupts();
      //for(n = size; n; --n){ wdt_reset(); eeprom_write_byte((uint8_t *)eeprom_addr++, *ptr++); }
//      eeprom_write_block((const void *)ptr, (void *)eeprom_addr, size);
      //interrupts();
      break;
    case SKIP:
      //eeprom_addr += size;
      break;
  }
  eeprom_addr += size;
}

template<typename T> void paramAction(uint8_t action, T& value, uint8_t menuid, const char *label, const char* enumArray[], int32_t _min, int32_t _max, bool continuous) {
  switch(action) {
    case UPDATE:
    case UPDATE_MENU:
      if(((int32_t)value + encoder_val) < _min) value = (continuous) ? _max : _min;
      else 
        if(((int32_t)value + encoder_val) > _max) value = (continuous) ? _min : _max;
        else value = (int32_t)value + encoder_val;
      encoder_val = 0;

      lcd.noCursor();
      printlabel(action, menuid, label);  // print normal/menu label
      if(enumArray == NULL) {  // print value
        if((_min < 0) && (value >= 0)) lcd.print('+');  // add + sign for positive values, in case negative values are supported
        lcd.print(value);
      } else {
        lcd.print(enumArray[value]);
      }
      lcd.blanks(); lcd.blanks(); //lcd.setCursor(0, 1);
      break;
    default:
      actionCommon(action, (uint8_t *)&value, sizeof(value));
      break;
  }
}

int8_t paramAction(uint8_t action, uint8_t id = ALL)  // list of parameters
{
  if((action == SAVE) || (action == LOAD)){
    eeprom_addr = EEPROM_OFFSET;
    for(uint8_t _id = 1; _id < id; _id++) paramAction(SKIP, _id);
  }
  
  if(id == ALL) for(id = 1; id != N_ALL_PARAMS+1; id++) paramAction(action, id);  // for all parameters

  switch(id) {    // Visible parameters
    case VOLUME:    paramAction(action, volume, 0x11, "Volume", NULL, -1, 16, false); break;
    case MODE:      paramAction(action, mode, 0x12, "Mode", mode_label, 0, _N(mode_label) - 1, false); break;
    case FILTER:    paramAction(action, filt, 0x13, "Filter BW", filt_label, 0, _N(filt_label) - 1, false); break;
    case BAND:      paramAction(action, bandval, 0x14, "Band", band_label, 0, _N(band_label) - 1, false); break;
    case STEP:      paramAction(action, stepsize, 0x15, "Tune Rate", stepsize_label, 0, _N(stepsize_label) - 1, false); break;
    case AGC:       paramAction(action, eeprom_version, 0x18, "AGC", offon_label, 0, 1, false); break;
    case NR:        paramAction(action, eeprom_version, 0x19, "NR", NULL, 0, 8, false); break;
    case ATT:       paramAction(action, eeprom_version, 0x1A, "ATT", att_label, 0, 7, false); break;
    case ATT2:      paramAction(action, eeprom_version, 0x1B, "ATT2", NULL, 0, 16, false); break;
    case SMETER:    paramAction(action, eeprom_version, 0x1C, "S-meter", smode_label, 0, _N(smode_label) - 1, false); break;
    case SWRMETER:  paramAction(action, eeprom_version, 0x1D, "SWR Meter", swr_label, 0, _N(swr_label) - 1, false); break;
    case CWDEC:     paramAction(action, eeprom_version, 0x21, "CW Decoder", offon_label, 0, 1, false); break;
    case KEY_TX:    paramAction(action, eeprom_version, 0x28, "Practice", offon_label, 0, 1, false); break;
    case DRIVE:     paramAction(action, eeprom_version, 0x33, "TX Drive", NULL, 0, 8, false); break;
    case MOX:       paramAction(action, eeprom_version, 0x35, "MOX", NULL, 0, 2, false); break;
    case PWM_MIN:   paramAction(action, pwm_min, 0x81, "PA Bias min", NULL, 0, pwm_max - 1, false); break;
    case PWM_MAX:   paramAction(action, pwm_max, 0x82, "PA Bias max", NULL, pwm_min, 255, false); break;
    case SI5351_FXTAL: paramAction(action, si5351_XTAL, 0x83, "Ref freq", NULL, 14000000, 28000000, false); break;

    // Invisible parameters
    case VERS:    paramAction(action, eeprom_version, 0, NULL, NULL, 0, 0, false); break;

    // Non-parameters
    case _NULL_PARAM:   menumode = 0; show_banner(); change = true; break;
    default:      if((action == NEXT_MENU) && (id != N_PARAMS)) id = paramAction(action, max(1, min(N_PARAMS, id + ((encoder_val > 0) ? 1 : -1))));
      break;
  }

  return id;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

