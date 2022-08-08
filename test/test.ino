/*
  By TF3LJ / VE2LJX

  https://sites.google.com/site/lofturj/power-and-swr-meter

  30:1 gives a coupling of 1/30, or [20log(1/30)] -29.5dB
  The voltage division gives a further attenuation of [20log(23.5/150.5)] -16.1dB
  The maximum input power specified for AD8307 is 17dBm.  Then the maximum input power the meter can handle is:
  17dBm + 16.1dB + 29.5 dB = 62.6 dBm or 32.6dBW, which is 1800W.

  While this adds the benefit of a simpler circuit and higher measurement 
  resolution (0.1 dB when using 10 bit ADC, 0.025 dB when using 12 bit ADC),
  it caps or limits the max AD8307 input to approximately 15.5 dBm, rather than the 17dBm as specified.
  A very acceptable trade off for the higher measurement resolution. 

  In other words, the highest possible input power reading when 
  using 30:1 transformers and a 127 ohm + 23.5 ohm (24 ohm in parallel with the internal 1100 ohm in the AD8307) 
  voltage divider is just over 1kW (1300W). 

  To be able to measure power levels above 500W (or 200W in reality, with high SWR levels)
  you may need to use larger cores for the Tandem Match coupler. 

  Using T68-2 or T68-3 cores and a 40:1 ratio, as described in the ARRL Antenna Book, or perhaps better,
  FT114-61, will make the meter suitable for power levels up to 2kW,
  without any modification required of the resistors R1 through R8.

  10 bit gives 5V/1024 = 4.88mV per bit, which equals 0.1952 dB measurement resolution

  dBm = 10 * log(P)              where P is power in milliwatts; or
  dBm = 10 * log(P) + 30         where P is power in watts
*/

#include "main.h"

// data_pin(rs)(orange), clk_pin (purpur) , enable_pin (red)
LiquidCrystal lcd(4, 5, 6);

void setup() {
  pinMode(ROT_A, INPUT);
  pinMode(ROT_B, INPUT);
  digitalWrite(ROT_A, HIGH);
  digitalWrite(ROT_B, HIGH); 

  attachInterrupt(0, int0, CHANGE);
  attachInterrupt(1, int0, CHANGE);

  pinMode(BUTTON_VCXO, INPUT);
  pinMode(BUTTON_STEP, INPUT);
  pinMode(BUTTON_TX, INPUT);
  pinMode(BUTTON_RIT, INPUT);
  pinMode(TX_OUT, OUTPUT);
  pinMode(tone_pin, OUTPUT); //объявляем пин как выход для звука
  pinMode(IF_WIDTH_PIN, OUTPUT); //объявляем пин как выход для изменения ширины ПЧ

  #ifdef BUTTON_TONE_ENABLE
      pinMode(BUTTON_TONE, INPUT_PULLUP);
  #endif

  #ifdef SWR
    // For the AD converters
    pinMode (analogInputFWD, INPUT); //input from AD8307
    pinMode (analogInputREF, INPUT); //input from AD8307
  #endif

  digitalWrite(BUTTON_VCXO, HIGH); 
  digitalWrite(BUTTON_STEP, HIGH); 
  digitalWrite(BUTTON_TX, HIGH); 
  digitalWrite(BUTTON_RIT, HIGH);
  digitalWrite(TX_OUT, LOW);
  
  Read_Value_EEPROM();

  si5351.init(SI5351_CRYSTAL_LOAD_0PF, SI5351_FXTAL, 0);

  switch (SI5351_DRIVE_CLK0) {
    case  2: si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA); break;
    case  4: si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA); break;
    case  6: si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_6MA); break;
    case  8: si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA); break;
  }

  switch (SI5351_DRIVE_CLK1) {
    case  2: si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA); break;
    case  4: si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_4MA); break;
    case  6: si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_6MA); break;
    case  8: si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA); break;
  }

  switch (SI5351_DRIVE_CLK2) {
    case  2: si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA); break;
    case  4: si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_4MA); break;
    case  6: si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_6MA); break;
    case  8: si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA); break;
  }
  
  si5351.set_freq((Frit + IF) * 100ULL, SI5351_CLK0); //Set RX
 
  lcd.begin(16, 2);  /* Инициализируем дисплей: 2 строки по 16 символов */
  display_vfo(Frit);
  lcd.setCursor(10,1);   lcd.print("  1kHz  ");
}

void int0() {
  encTick();
}

// алгоритм со сбросом от Ярослава Куруса
void encTick() {
  Enc_state = digitalRead(ROT_A) | digitalRead(ROT_B) << 1;  // digitalRead хорошо бы заменить чем-нибудь более быстрым
  if (resetFlag && Enc_state == 0b11) {
    if (Enc_last == 0b10) enc_move=1*ENC_SPIN;
    if (Enc_last == 0b01) enc_move=-1*ENC_SPIN;
    resetFlag = 0;
    enc_flag = true;
  }
  if (Enc_state == 0b00) resetFlag = 1;
  Enc_last = Enc_state;
}

void loop() {
  if (micros() - _100usTime > 100) {
    _100usTime = micros();

    #ifdef SWR
      if (tx_flag) {
        adc_poll(analogInputFWD, analogInputREF);
        determine_dBm();

        fwd_power_100us = pow(10, (fwd_dbm - 30) / 10.0);
        ref_power_100us = pow(10, (ref_dbm - 30) / 10.0);

        _100us_count++;
        if (_100us_count >= BUFER_MAX_5ms) _100us_count = 0;

        db_buff_max_5ms[_100us_count] = 100 * fwd_power_100us;

        uint32_t pk_5ms = 0;
        for (uint8_t i = 0; i < BUFER_MAX_5ms; i++) {
          if (pk_5ms < db_buff_max_5ms[i]) pk_5ms = db_buff_max_5ms[i];
        }

        fwd_power_5ms = pk_5ms / 100.0;
      }
    #endif
  }

  if (millis() - _5msTime > 5) {
    _5msTime = millis();
    _5msFunction();
  }

  if (millis() - _100msTime > 100) {
    _100msTime = millis();
    _100msFunction();
  }

  if (millis() - _1000msTime > 1000) {
    _1000msTime = millis();
    _1000msFunction();
  }
 
  if (step_flag) {
      if(enc_flag) {
        enc_flag = false;
        step_count = step_count + enc_move;
        if (step_count > 6) { step_count = 6;};
        if (step_count == 0) { step_count = 1;};
      }

      lcd.setCursor(10,1);
      switch (step_count) {
        case  1: STEP = 10;       lcd.print("  10"); break;   //10Hz
        case  2: STEP = 100;      lcd.print(" 100"); break;   //100Hz
        case  3: STEP = 1000;     lcd.print("  1k"); break;   //1kHz
        case  4: STEP = 5000;     lcd.print("  5k"); break;
        case  5: STEP = 10000;    lcd.print(" 10k"); break;   //10kHz
        case  6: STEP = 100000;   lcd.print("100k"); break;   //100kHz
      }

      lcd.print("Hz");
    return;
  }//End Step flag

  if(setup_flag) {
      F_setup();
  }
}//End loop

void CHECK_BUTTON_PRESS() {
  #ifdef BUTTON_TONE_ENABLE
      if(!digitalRead(BUTTON_TONE)) event = BT;
      
      if(event == BT && !BUTTON_TONE_FLAG)
      {
          BUTTON_TONE_FLAG = true;
          tone(tone_pin, Ftone);
      }

      if(event != BT && BUTTON_TONE_FLAG)
      {
          BUTTON_TONE_FLAG = false;
          noTone(tone_pin);
      }
  #endif

  event = 0;

  // Проверка кнопок
  if (Button_flag == 0) {
      if (digitalRead(BUTTON_TX) == 0) {
        Button_flag = true;
        F_tx();
        return;
      }//End BUTTON_TX

      if (digitalRead(BUTTON_STEP) == 0) {
        Button_flag = true;
        
        if(!setup_flag) enc_block = !enc_block;

        step_flag = !step_flag;
        return;
      }//End BUTTON_STEP

      if (digitalRead(BUTTON_VCXO) == 0 && !tx_flag) {
        Button_flag = true;                 
        vcxo_flag = !vcxo_flag;
        F_if2();
        return;
      }//End BUTTON_VCXO
      
      if (digitalRead(BUTTON_RIT) == 0 && !tx_flag) {
        Button_flag = true;
        rit_flag = !rit_flag;

        uint32_t t0 = millis();
        for(; !digitalRead(BUTTON_RIT);) {
          if((millis() - t0) > 300) {
            setup_flag = !setup_flag;

            if(setup_flag) {
              lcd.clear();
              enc_flag = true;
              enc_block = true;
              rit_flag = true;

              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("Setup");
              delay(400);
              lcd.clear();

              F_setup();
            } else {
              lcd.clear();
              enc_block = false;
              rit_flag = false;
              display_vfo(Frit);
              setup_flag = false;
            }

            t0 = 0;
            break;
          }
        }

        if (!setup_flag) F_rit();
      }//End BUTTON_RIT
  }

  if (digitalRead(BUTTON_TX) == 1 && digitalRead(BUTTON_STEP) == 1 && digitalRead(BUTTON_VCXO) == 1 && digitalRead(BUTTON_RIT) == 1 && Button_flag == true) {
    Button_flag = false;
  }
//конец проверок кнопок

  if (enc_flag && enc_block == 0) {
    if(rit_flag == false) {
      Ftx += STEP*enc_move;
        if (Ftx < Fmin) Ftx = Fmin;
        if (Ftx > Fmax) Ftx = Fmax;
      Frit = Ftx;
    } else {
      Frit = Frit + STEP*enc_move;
      if (Frit < Fmin) Frit = Fmin;
      if (Frit > Fmax) Frit = Fmax;
    }

    if (tx_flag == true) {
      si5351.set_freq(Ftx*xF * 100ULL, SI5351_CLK1 );
      si5351.set_freq((Frit + IF) * 100ULL, SI5351_CLK0 );
    } else {
      si5351.set_freq((Frit + IF) * 100ULL, SI5351_CLK0 );
    }

    enc_flag = false;
    display_vfo(Frit);
  }
}

void Read_Value_EEPROM() {
/*
  EEPROM
  0-3   ( 4 Byte) IF
  4     ( 1 Byte)
  5-7    NULL
  8-11  ( 4 Byte) Fmin
  12-15 ( 4 Byte) Fmax
  16-19 ( 4 Byte) SI5351_FXTAL
  20-23 ( 4 Byte) Ftx
  24-25 ( 2 Byte) Ftone
  26    ( 1 Byte) SI5351_DRIVE_CLK0   2MA 4MA 6MA 8MA 
  27    ( 1 Byte) SI5351_DRIVE_CLK1
  28    ( 1 Byte) SI5351_DRIVE_CLK2
  29    ( 1 Byte) ENC_SPIN
  30-33 ( 4 Byte) IF2
  34    ( 1 Byte) x*F
  35    ( 1 Byte) IF_WIDTH_FLAG
  36-37 ( 2 Byte) ATT_PWR_METER
  38-39 ( 2 Byte) FWR_ERROR
  40-41 ( 2 Byte) REF_ERROR
*/
      EEPROM.get(0, IF);     //Первая ПЧ
      if (IF > 120000000 || IF < 0) {
        IF = 0;
        EEPROM.put(0, IF);
      }
      EEPROM.get(8, Fmin);   //Fmin = 10кГц
      if (Fmin > 40000000 || Fmin < 0) {
        Fmin = 10000;
        EEPROM.put(8, Fmin);
      }
      EEPROM.get(12, Fmax);  //F max 30 MHz  30.000.000.00
      if (Fmax > 4000000000) {
        Fmax = 999000000;
        EEPROM.put(12, Fmax);
      }
      EEPROM.get(16, SI5351_FXTAL); //SI5351_FXTAL = 0;
      if (SI5351_FXTAL > 32000000) {
          SI5351_FXTAL = 32000000;
          EEPROM.put(16, SI5351_FXTAL);          
      }
      if (SI5351_FXTAL < 14000000){
          SI5351_FXTAL = 14000000;
          EEPROM.put(16, SI5351_FXTAL);
      }
      EEPROM.get(20, Ftx);
      if (Ftx > 150*1e6){
        Ftx = 3.1*1e6;
        EEPROM.put(20, Ftx);
      }
      EEPROM.get(24, Ftone); //Ftone  = 1000;
      if (Ftone > 60000U){
        Ftone = 1000;
        EEPROM.put(24, Ftone);
      }
      EEPROM.get(26, SI5351_DRIVE_CLK0); // Driver current
      if (SI5351_DRIVE_CLK0 > 8){
        SI5351_DRIVE_CLK0 = 4;
        EEPROM.put(26, SI5351_DRIVE_CLK0);
      }
      EEPROM.get(27, SI5351_DRIVE_CLK1); // Driver current
      if (SI5351_DRIVE_CLK1 > 8){
        SI5351_DRIVE_CLK1 = 4;
        EEPROM.put(27, SI5351_DRIVE_CLK1);
      }
      EEPROM.get(28, SI5351_DRIVE_CLK2); // Driver current
      if (SI5351_DRIVE_CLK2 > 8){
        SI5351_DRIVE_CLK2 = 4;
        EEPROM.put(28, SI5351_DRIVE_CLK2);
      }
      EEPROM.get(29, ENC_SPIN); // Driver current
      if (ENC_SPIN > 1 || ENC_SPIN < -1){
        ENC_SPIN = 1;
        EEPROM.put(29, ENC_SPIN);
      }
      EEPROM.get(30, IF2);
      if (IF2 > 4000000000){
        IF2 = 45000000;
        EEPROM.put(30, IF2);
      }
      EEPROM.get(34, xF); //xF  = 1; multiplier F
      if (xF > 2){
        xF = 2;
        EEPROM.put(34, xF);
      }

      EEPROM.get(35, IF_WIDTH_FLAG); // Ширина ПЧ
      if (IF_WIDTH_FLAG > 1 || IF_WIDTH_FLAG < 0) {
        IF_WIDTH_FLAG = 0;
        EEPROM.put(35, IF_WIDTH_FLAG);
      }

      #ifdef SWR
        EEPROM.get(36, ATT_PWR_METER);
        if (ATT_PWR_METER > 10000 || ATT_PWR_METER < -10000) {
          ATT_PWR_METER = 0;
          EEPROM.put(36, ATT_PWR_METER);
        }
        AttPwrMeter = ATT_PWR_METER / 100.0;

        EEPROM.get(38, FWR_ERROR);
        if (FWR_ERROR > 10000 || FWR_ERROR < -10000) {
          FWR_ERROR = 0;
          EEPROM.put(38, FWR_ERROR);
        }
        calibrateFRW = FWR_ERROR / 100.0;

        EEPROM.get(40, REF_ERROR);
        if (REF_ERROR > 10000 || REF_ERROR < -10000) {
          REF_ERROR = 0;
          EEPROM.put(40, REF_ERROR);
        }
        calibrateREF = REF_ERROR / 100.0;
      #endif

      STEP = 1000;
      Frit = Ftx;
}// End Read EEPROM

void F_tx() {
  if(enc_block) return;

  tx_flag = !tx_flag;

  if (tx_flag == true) {
    si5351.set_freq(Ftx*xF * 100ULL, SI5351_CLK1 );
    si5351.output_enable(SI5351_CLK1, 1);
    lcd.setCursor(0,1);
    lcd.print("TX  ");
    digitalWrite(TX_OUT, HIGH);
  }
  else {
    digitalWrite(TX_OUT, LOW);

    lcd.setCursor(10, 0);
    lcd.print("      ");

    lcd.setCursor(0, 1);
    lcd.print("                ");

    lcd.setCursor(10,1);
    switch (step_count) {
      case  1: STEP = 10;       lcd.print("  10"); break;   //10Hz
      case  2: STEP = 100;      lcd.print(" 100"); break;   //100Hz
      case  3: STEP = 1000;     lcd.print("  1k"); break;   //1kHz
      case  4: STEP = 5000;     lcd.print("  5k"); break;
      case  5: STEP = 10000;    lcd.print(" 10k"); break;   //10kHz
      case  6: STEP = 100000;   lcd.print("100k"); break;   //100kHz
    }

    lcd.print("Hz");

    si5351.output_enable(SI5351_CLK1, 0);
  }
}//End F tx

void F_if2() {
  if (vcxo_flag == true) {
      si5351.set_freq(IF2, SI5351_CLK2 );
      si5351.output_enable(SI5351_CLK2, 1);
      lcd.setCursor(14,0);
      lcd.print("IF");
  } else {
      si5351.output_enable(SI5351_CLK2, 0);
      lcd.setCursor(13,0);
      lcd.print("   "); 
  }
}//End F if

void F_rit() {
  if (rit_flag == true) {
    lcd.setCursor(10,0);
    lcd.print("RIT");
  } else {
    Frit = Ftx;
    si5351.set_freq((Frit + IF) * 100ULL, SI5351_CLK0 );
    lcd.setCursor(10,0);
    lcd.print("   ");
    display_vfo(Frit); 
  }
}// End F rit

void F_setup() {
  if (enc_flag) {
      if(rit_flag) {
              setup_count = setup_count + enc_move;
              if (setup_count > 19) { setup_count = 19;};
              if (setup_count == 0) { setup_count = 1;};
      }
      else {
        switch (setup_count) {
          case 1: IF += STEP*enc_move; if(IF > 100e6) IF = 100e6; if(IF < 0) IF = 0; break;
          case 2: Fmin += STEP*enc_move; if(Fmin < 10000) Fmin = 10000; break;
          case 3: Fmax += STEP*enc_move; if(Fmax > 999e6) Fmax = 999e6; break;
          case 4:
              SI5351_FXTAL += (STEP) * enc_move; 
              if(SI5351_FXTAL > 32e6) SI5351_FXTAL=32e6; 
              if(SI5351_FXTAL < 10e6) SI5351_FXTAL=10e6;
            break;
          case 5: Ftx += STEP*enc_move; if(Ftx > Fmax) Ftx=Fmax; if(Ftx < Fmin) Ftx=Fmin; break;
          case 6: Ftone += (STEP/10)*enc_move; break;
          case 7: if(enc_move == 1){ ftone_flag = true; tone(tone_pin, Ftone); } if(enc_move == -1){ ftone_flag = false; noTone(tone_pin); }; break;
          case 8: if(enc_move == 1) { 
              F_eeprom_w();
              lcd.setCursor(0,1);
              lcd.print("    Complite!!! ");
              delay(300);
              rit_flag = true;
            }; 
            break; // EEPROM write
          case 9: if(enc_move == 1) softReset(); break; // soft reboot
          case 10: if(enc_move == 1) xF = 2; if(enc_move == -1) xF = 1; break; // xF
          case 11: SI5351_DRIVE_CLK0+=enc_move*2; if(SI5351_DRIVE_CLK0 < 2) SI5351_DRIVE_CLK0 = 2; if(SI5351_DRIVE_CLK0 > 8) SI5351_DRIVE_CLK0 = 8;   break;
          case 12: SI5351_DRIVE_CLK1+=enc_move*2; if(SI5351_DRIVE_CLK1 < 2) SI5351_DRIVE_CLK1 = 2; if(SI5351_DRIVE_CLK1 > 8) SI5351_DRIVE_CLK1 = 8;   break;
          case 13: SI5351_DRIVE_CLK2+=enc_move*2; if(SI5351_DRIVE_CLK2 < 2) SI5351_DRIVE_CLK2 = 2; if(SI5351_DRIVE_CLK2 > 8) SI5351_DRIVE_CLK2 = 8;   break;
          case 14: if(enc_move == 1) ENC_SPIN = 1; if(enc_move == -1) ENC_SPIN = -1; break;
          case 15: IF2+=(STEP*100)*enc_move; if(IF2 > 4200000000) IF2 = 0; if(IF2 > 4000000000) IF2 = 4000000000; break;
          case 16: if(enc_move == 1) { IF_WIDTH_FLAG = true; digitalWrite(IF_WIDTH_PIN, HIGH);}; if(enc_move == -1) { IF_WIDTH_FLAG = false; digitalWrite(IF_WIDTH_PIN, LOW);}; break;
          case 17: ATT_PWR_METER += (STEP/10)*enc_move; AttPwrMeter = ATT_PWR_METER / 100.0;
            break;
          case 18: FWR_ERROR += (STEP/10)*enc_move; calibrateFRW = FWR_ERROR / 100.0;
            break;
          case 19: REF_ERROR += (STEP/10)*enc_move; calibrateREF = REF_ERROR / 100.0;
            break;
        }
      }

      lcd.clear();
      lcd.setCursor(0,0);
      switch (setup_count) {
        case 1:   lcd.print("IF");lcd.setCursor(0,1);lcd.print(IF); break;
        case 2:   lcd.print("Fmin");lcd.setCursor(0,1);lcd.print(Fmin); break;
        case 3:   lcd.print("Fmax");lcd.setCursor(0,1);lcd.print(Fmax); break;
        case 4:   lcd.print("SI5351_FXTAL");lcd.setCursor(0,1);lcd.print(SI5351_FXTAL); break;
        case 5:   lcd.print("Ftx");lcd.setCursor(0,1);lcd.print(Ftx); break;
        case 6:   lcd.print("Ftone");lcd.setCursor(0,1);lcd.print(Ftone); break;
        case 7:   lcd.print("Ftone On/Off");lcd.setCursor(0,1);if( ftone_flag) lcd.print("On"); else lcd.print("Off"); break;
        case 8:   lcd.print("EEPROM Write"); lcd.setCursor(0,1); lcd.print("No/Yes?"); break;
        case 9:   lcd.print("Reboot No/Yes?"); break;
        case 10:  lcd.print("xF"); lcd.setCursor(0,1);lcd.print(xF);break;
        case 11:  lcd.print("DRIVE_CLK0"); lcd.setCursor(0,1);lcd.print(SI5351_DRIVE_CLK0);break;
        case 12:  lcd.print("DRIVE_CLK1"); lcd.setCursor(0,1);lcd.print(SI5351_DRIVE_CLK1);break;
        case 13:  lcd.print("DRIVE_CLK2"); lcd.setCursor(0,1);lcd.print(SI5351_DRIVE_CLK2);break;
        case 14:  lcd.print("ENC_SPIN"); lcd.setCursor(0,1);lcd.print(ENC_SPIN);break;
        case 15:  lcd.print("IF2");
                  lcd.setCursor(0,1);lcd.print(IF2/100);
          break;
        case 16:  lcd.print("1.6 IF_WIDTH_PIN");
                  lcd.setCursor(0,1); if (IF_WIDTH_FLAG) lcd.print("ON"); else lcd.print("OFF");
          break;
        case 17:  lcd.print("1.7 ATT_PWR(db)");
                  lcd.setCursor(0, 1);
                  lcd.print(AttPwrMeter);
          break;
        case 18:  lcd.print("1.8 FWR Error");
                  lcd.setCursor(0, 1);
                  lcd.print(calibrateFRW);
          break;
        case 19:  lcd.print("1.9 Ref Error");
                  lcd.setCursor(0, 1);
                  lcd.print(calibrateREF);
          break;
      }
    enc_flag = false;
  }
}// End F setup

void F_eeprom_w() {

unsigned long temp=0;
long temp_l=0;

      EEPROM.get(0, temp);     //IF=45000000;        // Расчетная промежуточная частота. 450.000.00
      if (IF != temp){
        EEPROM.put(0, IF);
      }
      EEPROM.get(8, temp);   //Fmin = 30000000;            // min 300kHz   
      if (Fmin != temp){
        EEPROM.put(8, Fmin);
      }
      EEPROM.get(12, temp);  //Fmax = 3000000000;          // max 30 MHz  30.000.000.00
      if (Fmax != temp){
        EEPROM.put(12, Fmax);
      }
      
      EEPROM.get(16, temp_l);
      if (SI5351_FXTAL != temp_l){
        EEPROM.put(16, SI5351_FXTAL);
      }
      
      EEPROM.get(20, temp);
      if (Ftx != temp){
        EEPROM.put(20, Ftx);
      }
      
      temp = Ftone;
      EEPROM.get(24, Ftone); //Ftone  = 1000;
      if (Ftone != temp){
        Ftone = temp;
        EEPROM.put(24, Ftone);
      }
            
      temp = SI5351_DRIVE_CLK0;
      EEPROM.get(26, SI5351_DRIVE_CLK0); // Driver current
      if (SI5351_DRIVE_CLK0 != temp){
        SI5351_DRIVE_CLK0 = temp;
        EEPROM.put(26, SI5351_DRIVE_CLK0);
      }
      
      temp = SI5351_DRIVE_CLK1;
      EEPROM.get(27, SI5351_DRIVE_CLK1); // Driver current
      if (SI5351_DRIVE_CLK1 != temp){
        SI5351_DRIVE_CLK1 = temp;
        EEPROM.put(27, SI5351_DRIVE_CLK1);
      }
      
      temp = SI5351_DRIVE_CLK2;
      EEPROM.get(28, SI5351_DRIVE_CLK2); // Driver current
      if (SI5351_DRIVE_CLK2 != temp){
        SI5351_DRIVE_CLK2 = temp;
        EEPROM.put(28, SI5351_DRIVE_CLK2);
      }

      temp = ENC_SPIN;
      EEPROM.get(29, ENC_SPIN);
      if (ENC_SPIN != temp){
        ENC_SPIN = temp;
        EEPROM.put(29, ENC_SPIN);
      }

      EEPROM.get(30, temp);
      if (IF2 != temp){
        EEPROM.put(30, IF2);
      }

      temp = xF;
      EEPROM.get(34, xF); //xF  = 1; multiplier F
      if (xF != temp){
        xF = temp;
        EEPROM.put(34, xF);
      }

      temp = IF_WIDTH_FLAG;
      EEPROM.get(35, IF_WIDTH_FLAG); // Ширина ПЧ
      if (IF_WIDTH_FLAG != temp) {
        IF_WIDTH_FLAG = temp;
        EEPROM.put(35, IF_WIDTH_FLAG);
      }

      #ifdef SWR
        temp = ATT_PWR_METER;
        EEPROM.get(36, ATT_PWR_METER); // ATT PWR Meter
        if (ATT_PWR_METER != temp) {
          ATT_PWR_METER = temp;
          EEPROM.put(36, ATT_PWR_METER);
        }

        temp = FWR_ERROR;
        EEPROM.get(38, FWR_ERROR); // FWR_ERROR
        if (FWR_ERROR != temp) {
          FWR_ERROR = temp;
          EEPROM.put(38, FWR_ERROR);
        }

        temp = REF_ERROR;
        EEPROM.get(40, REF_ERROR); // REF_ERROR
        if (REF_ERROR != temp) {
          REF_ERROR = temp;
          EEPROM.put(40, REF_ERROR);
        }
      #endif
}

void display_vfo(int32_t f) {
  lcd.noCursor();
  lcd.setCursor(0, 0);

  int32_t scale = 1e8;
  if(f/scale == 0) { lcd.print(' '); scale /= 10; }
  if(f/scale == 0) { lcd.print(' '); scale /= 10; }

  for(; scale != 1; f %= scale, scale /= 10) {
    lcd.print(char('0' + f/scale));
    if(scale == (int32_t)1e3 || scale == (int32_t)1e6) lcd.print('.');
  }
}

void softReset() {
  asm volatile ("  jmp 0");
}

void _5msFunction() {
  CHECK_BUTTON_PRESS();

  #ifdef SWR
    if (tx_flag) {
      // Instantaneous forward voltage and power, milliwatts and dBm
      v_fwd = pow(10, fwd_dbm/20.0);		        // (We use voltage later on, for SWR calc)
      // Instantaneous reverse voltage and power
      v_ref = pow(10, ref_dbm/20.0);

      calculate_pk_pep_avg(v_fwd);                  // Determine Peak, PEP and AVG
      swr = calculate_SWR(v_fwd, v_ref);            // and determine SWR
    }
  #endif
}

void _100msFunction() {
  if (tx_flag) {
    #ifdef SWR
      _1s_count++;
      if (_1s_count >= PEP_BUFFER) _1s_count = 0;
      db_buff_1s[_1s_count] = power_pk_100ms;

      // Retrieve Max Value within a 1 second sliding window
      uint16_t pep = 0;
      for (uint8_t x = 0; x < PEP_BUFFER; x++) {
        if (pep < db_buff_1s[x]) pep = db_buff_1s[x];
      }
      power_pep_1s = pep;

      smeter_cnt++;
      if(smeter_cnt > 2) {
        smeter_cnt = 0;

        lcd.setCursor(11, 0);
        lcd.print(Vfwd);
        //lcd.print("AM   ");
        //lcd.setCursor(13, 0);
        //lcd.print((uint8_t)modulation_index);

        lcd.setCursor(0, 1);
        lcd.print("P       ");
        lcd.setCursor(2, 1);
        //lcd.print((int)power_pk_100ms);
        lcd.print(power_pep_1s);
        //lcd.print(fwd_power_5ms);
        //lcd.print(fwd_dbm);

        lcd.setCursor(7, 1);
        lcd.print(" SWR ");
        lcd.print("     ");
        lcd.setCursor(12, 1);
        lcd.print(swr);
      }
    #endif
  } else {
    if (setup_flag) return;

    if (vcxo_flag == true) {
        lcd.setCursor(14,0);
        lcd.print("IF");
    }

    if (rit_flag == true) {
      lcd.setCursor(10,0);
      lcd.print("RIT");
    }


    uint16_t tmp = AdcReadAvrValue(Smeter);
    uSMETER = (tmp + uSMETER)/2;

    smeter_cnt++;
    if(smeter_cnt > 2) {
      smeter_cnt = 0;
      lcd.setCursor(0,1);
      lcd.print("    ");
      lcd.setCursor(0,1);
      lcd.print(uSMETER);
    }
  }
}

void _1000msFunction() {
}
