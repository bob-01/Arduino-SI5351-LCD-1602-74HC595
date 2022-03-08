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
  
  si5351.set_freq(Frit+IF, SI5351_CLK0); //Set RX
 
  lcd.begin(16, 2);  /* Инициализируем дисплей: 2 строки по 16 символов */
  F_print();
  lcd.setCursor(10,1);   lcd.print("  1kHz  ");

  currentTime = millis();
  loopTime = currentTime; 
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
  currentTime = millis();

  // Проверка каждые 5 мс
  if(currentTime >= (loopTime + 5)) {
    CHECK_BUTTON_PRESS();        
    smeter_count++;
    loopTime = currentTime; // Счетчик прошедшего времени      
  }

    if ( (smeter_count % 100 == 0) && !setup_flag) {
        count_avr++;

        if (!tx_flag) {
            uSMETER += analogRead(Smeter);
        }

        if (count_avr >= 10) {
          smeter_count = 0;
          count_avr = 0;

          if (!tx_flag) {
            uSMETER = uSMETER/10;
            lcd.setCursor(0,1);
            lcd.print(uSMETER); 
            lcd.print("   ");
          }
        }//End  if (count_avr > 3) 
    }//End s_meter
    
    if (step_flag) {
        if(enc_flag)
        {
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
      }//End BUTTON_TX

      if (digitalRead(BUTTON_STEP) == 0) {
        Button_flag = true;
        
        if(!setup_flag) enc_block = !enc_block;

        step_flag = !step_flag;
      }//End BUTTON_STEP

      if (digitalRead(BUTTON_VCXO) == 0) {
        Button_flag = true;                 
        vcxo_flag = !vcxo_flag;
        F_if2();
      }//End BUTTON_VCXO
      
      if (digitalRead(BUTTON_RIT) == 0) {
        Button_flag = true;                 
        rit_flag = !rit_flag;
        menu_count = 0;
      }//End BUTTON_RIT
  }

  if (digitalRead(BUTTON_TX) == 1 && digitalRead(BUTTON_STEP) == 1 && digitalRead(BUTTON_VCXO) == 1 && digitalRead(BUTTON_RIT) == 1 && Button_flag == true) {
      Button_flag = false;
    if ( menu_count > 0 && menu_count < 128 && setup_flag == false){
      F_rit();
    }
  }

  if(digitalRead(BUTTON_RIT) == 0 && Button_flag == true && menu_count < 255 ) {
      menu_count++;
    if (menu_count == 254) {
      if(setup_flag) {
        lcd.clear();
        enc_block = false;
        F_print();
        step_flag = 1;
        setup_flag = false; 
      } else {
        enc_flag = true;
        F_setup();
      }
    }
  }
//конец проверок кнопок

  if (enc_flag && enc_block == 0) {
    if(rit_flag == false) {
      Ftx += (STEP*100)*enc_move;
        if (Ftx < Fmin){ Ftx = Fmin; }
        if (Ftx > Fmax){ Ftx = Fmax; }
      Frit = Ftx;
    } else {
      Frit = Frit + (STEP*100)*enc_move;
      if (Frit < Fmin){ Frit = Fmin; }
      if (Frit > Fmax){ Frit = Fmax; }
    }

    if (tx_flag == true) {
      si5351.set_freq(Ftx*xF, SI5351_CLK1 );
      si5351.set_freq(Frit+IF, SI5351_CLK0 );
    } else {
      si5351.set_freq(Frit+IF, SI5351_CLK0 );
    }

    enc_flag = false;
    F_print();
  }
}

void F_print() {
  uint16_t mid;
  //3.110.000.00
  //9999.000.00
  mid = Frit/100000;    //3.110.
  lcd.setCursor(0,0);
  lcd.write(' ');
                  
  if ( (mid/1000) > 9) {
    lcd.setCursor(0,0);
  }

  lcd.print(mid/1000);
  lcd.write('.');
  
  mid = (mid%1000);
  lcd.print(mid/100);
  mid = (mid%100);
  lcd.print(mid/10);
  lcd.print( mid%10);
  lcd.write('.');
  
  mid = (Frit/1000)%100;

  lcd.print( mid/10);
  lcd.print( mid%10);
}//end f_print

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
*/
      EEPROM.get(0, IF);     //Первая ПЧ
      if (IF > 4000000000){
        IF = 1070000000ULL;
        EEPROM.put(0, IF);
      }
      EEPROM.get(8, Fmin);   //Fmin = 10кГц
      if (Fmin > 4000000000) {
        Fmin = 1000000;
        EEPROM.put(8, Fmin);
      }
      EEPROM.get(12, Fmax);  //F max 30 MHz  30.000.000.00
      if (Fmax > 4000000000) {
        Fmax = 4000000000;
        EEPROM.put(12, Fmax);
      }
      EEPROM.get(16, SI5351_FXTAL); //SI5351_FXTAL = 0;
      if (SI5351_FXTAL > 28000000) {
          SI5351_FXTAL = 28000000;
          EEPROM.put(16, SI5351_FXTAL);          
      }
      if (SI5351_FXTAL < 14000000){
          SI5351_FXTAL = 14000000;
          EEPROM.put(16, SI5351_FXTAL);
      }
      EEPROM.get(20, Ftx);   //Ftx  = 311000000;
      if (Ftx > 4000000000){
        Ftx = 310000000;
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

      STEP = 1000;
      Frit = Ftx;
}// End Read EEPROM

void F_tx() {
  if(enc_block) return;

  tx_flag = !tx_flag;

  if (tx_flag == true) {
    si5351.set_freq(Ftx*xF, SI5351_CLK1 );
    si5351.output_enable(SI5351_CLK1, 1);
    lcd.setCursor(0,1);
    lcd.print("TX  ");
    digitalWrite(TX_OUT, HIGH);
  }
  else {
    digitalWrite(TX_OUT, LOW);
    lcd.setCursor(0,1);
    lcd.print("   ");
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
    si5351.set_freq(Frit+IF, SI5351_CLK0 );
    lcd.setCursor(10,0);
    lcd.print("   ");
    F_print(); 
  }
}// End F rit

void F_setup() {
  if (setup_flag == false) {
    enc_block = true;
    setup_flag = true;

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Setup");
    delay(400);
    lcd.clear();
  }

    if (enc_flag) {
        if(rit_flag) {
               setup_count = setup_count+enc_move;
               if (setup_count > 16) { setup_count = 16;};
               if (setup_count == 0) { setup_count = 1;};
        }
        else {
          switch (setup_count) {
            case 1: IF+=(STEP*100)*enc_move; if(IF > 4200000000) IF = 0; if(IF > 4000000000) IF = 4000000000; break;
            case 2: Fmin+=(STEP*100)*enc_move; if(Fmin < 250000) Fmin=250000; break;
            case 3: Fmax+=(STEP*100)*enc_move; if(Fmax > 4000000000) Fmax=4000000000; break;
            case 4: SI5351_FXTAL+=(STEP)*enc_move; if(SI5351_FXTAL > 28*1e6) SI5351_FXTAL=28*1e6; if(SI5351_FXTAL < 14*1e6) SI5351_FXTAL=14*1e6;  break;
            case 5: Ftx+=(STEP*100)*enc_move; if(Ftx > Fmax) Ftx=Fmax; if(Ftx < Fmin) Ftx=Fmin; break;
            case 6: Ftone+=(STEP/10)*enc_move; break;
            case 7: if(enc_move == 1){ ftone_flag = true; tone(tone_pin, Ftone); } if(enc_move == -1){ ftone_flag = false; noTone(tone_pin); }; break;
            case 8: if(enc_move == 1) { F_eeprom_w(); lcd.setCursor(0,1); lcd.print("    Complite!!! "); delay (1000); }; break; // EEPROM write
            case 9: if(enc_move == 1) softReset(); break; // soft reboot
            case 10: if(enc_move == 1) xF = 2; if(enc_move == -1) xF = 1; break; // xF
            case 11: SI5351_DRIVE_CLK0+=enc_move*2; if(SI5351_DRIVE_CLK0 < 2) SI5351_DRIVE_CLK0 = 2; if(SI5351_DRIVE_CLK0 > 8) SI5351_DRIVE_CLK0 = 8;   break;
            case 12: SI5351_DRIVE_CLK1+=enc_move*2; if(SI5351_DRIVE_CLK1 < 2) SI5351_DRIVE_CLK1 = 2; if(SI5351_DRIVE_CLK1 > 8) SI5351_DRIVE_CLK1 = 8;   break;
            case 13: SI5351_DRIVE_CLK2+=enc_move*2; if(SI5351_DRIVE_CLK2 < 2) SI5351_DRIVE_CLK2 = 2; if(SI5351_DRIVE_CLK2 > 8) SI5351_DRIVE_CLK2 = 8;   break;
            case 14: if(enc_move == 1) ENC_SPIN = 1; if(enc_move == -1) ENC_SPIN = -1; break;
            case 15: IF2+=(STEP*100)*enc_move; if(IF2 > 4200000000) IF2 = 0; if(IF2 > 4000000000) IF2 = 4000000000; break;
            case 16: if(enc_move == 1) { IF_WIDTH_FLAG = true; digitalWrite(IF_WIDTH_PIN, HIGH);}; if(enc_move == -1) { IF_WIDTH_FLAG = false; digitalWrite(IF_WIDTH_PIN, LOW);}; break;
          }
        }

        lcd.clear();
        lcd.setCursor(0,0);

        switch (setup_count) {
          case 1:   lcd.print("IF");lcd.setCursor(0,1);lcd.print(IF/100); break;
          case 2:   lcd.print("Fmin");lcd.setCursor(0,1);lcd.print(Fmin/100); break;
          case 3:   lcd.print("Fmax");lcd.setCursor(0,1);lcd.print(Fmax/100); break;
          case 4:   lcd.print("SI5351_FXTAL");lcd.setCursor(0,1);lcd.print(SI5351_FXTAL); break;
          case 5:   lcd.print("Ftx");lcd.setCursor(0,1);lcd.print(Ftx/100); break;
          case 6:   lcd.print("Ftone");lcd.setCursor(0,1);lcd.print(Ftone); break;
          case 7:   lcd.print("Ftone On/Off");lcd.setCursor(0,1);if( ftone_flag) lcd.print("On"); else lcd.print("Off"); break;
          case 8:   lcd.print("EEPROM Write"); lcd.setCursor(0,1); lcd.print("No/Yes?"); break;
          case 9:  lcd.print("Reboot No/Yes?"); break;
          case 10:  lcd.print("xF"); lcd.setCursor(0,1);lcd.print(xF);break;
          case 11:  lcd.print("DRIVE_CLK0"); lcd.setCursor(0,1);lcd.print(SI5351_DRIVE_CLK0);break;
          case 12:  lcd.print("DRIVE_CLK1"); lcd.setCursor(0,1);lcd.print(SI5351_DRIVE_CLK1);break;
          case 13:  lcd.print("DRIVE_CLK2"); lcd.setCursor(0,1);lcd.print(SI5351_DRIVE_CLK2);break;
          case 14:  lcd.print("ENC_SPIN"); lcd.setCursor(0,1);lcd.print(ENC_SPIN);break;
          case 15:  lcd.print("IF2");lcd.setCursor(0,1);lcd.print(IF2/100); break;
          case 16:  lcd.print("IF_WIDTH_PIN"); lcd.setCursor(0,1); if (IF_WIDTH_FLAG) lcd.print("ON"); else lcd.print("OFF"); break;
        }
      enc_flag = false;
    }
}// End F setup

void F_eeprom_w(){

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
      
      EEPROM.get(20, temp);   //Ftx  = 311000000;
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
}

void softReset(){
  asm volatile ("  jmp 0");
}
