/*
  15.12.2017 Добавляю задержку прием/передача
*/

#include "si5351.h"
#include "Wire.h"
#include "ShiftedLCD.h"
#include <EEPROM.h> // Для работы со встроенной памятью ATmega

#define pin_A          3
#define pin_B          2

#define Button_TX      7 
#define Button_Step    8 
#define Button_VCXO    9

#define Button_RIT     10
#define TX_out         11
#define tone_pin       12

#define Smeter         A0
#define FRD            A3
#define REV            A2

Si5351 si5351;

// data_pin(rs)(orange), clk_pin (purpur) , enable_pin (red)
LiquidCrystal lcd(4,5,6);

unsigned long currentTime,loopTime;
unsigned long IF,IF2, XTAL, Fmin, Fmax, Ftx, Frit, STEP;
long Fcorr;

boolean enc_block=false, enc_flag=false, rit_flag=false, Button_flag=false, tx_flag=false, vcxo_flag=false, step_flag=false, rewrite_flag=false, setup_flag=false, ftone_flag=false;

uint8_t count_avr=0, smeter_count=0, Enc_state, Enc_last, step_count=3, menu_count=0, setup_count=8, xF=1, SI5351_DRIVE_CLK0, SI5351_DRIVE_CLK1, SI5351_DRIVE_CLK2, SI5351_CAPACITOR_LOAD; // 0 ..255
int8_t enc_move=0, mode=1;

// Mode = 1 RX
// Mode = 2 TX
// Mode = 1 Setup

uint16_t Ftone,uFRD,uREV,uSMETER;

void setup(){

  pinMode(pin_A, INPUT);
  pinMode(pin_B, INPUT);
  pinMode(Button_VCXO, INPUT);
  pinMode(Button_Step, INPUT);
  pinMode(Button_TX, INPUT);
  pinMode(Button_RIT, INPUT);
  pinMode(TX_out, OUTPUT);
  pinMode(tone_pin, OUTPUT); //объявляем пин как выход для звука

  digitalWrite(pin_A, HIGH);
  digitalWrite(pin_B, HIGH); 
  digitalWrite(Button_VCXO, HIGH); 
  digitalWrite(Button_Step, HIGH); 
  digitalWrite(Button_TX, HIGH); 
  digitalWrite(Button_RIT, HIGH);
  digitalWrite(TX_out, LOW);
    
  Read_Value_EEPROM();

  switch (SI5351_CAPACITOR_LOAD){
                             case  0: si5351.init(SI5351_CRYSTAL_LOAD_0PF, XTAL, Fcorr); break;
                             case  6: si5351.init(SI5351_CRYSTAL_LOAD_6PF, XTAL, Fcorr); break;
                             case  8: si5351.init(SI5351_CRYSTAL_LOAD_8PF, XTAL, Fcorr); break;
                             case  10: si5351.init(SI5351_CRYSTAL_LOAD_10PF, XTAL, Fcorr); break;
  }

  switch (SI5351_DRIVE_CLK0){
                             case  2: si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA); break;
                             case  4: si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA); break;
                             case  6: si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_6MA); break;
                             case  8: si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA); break;
  }

  switch (SI5351_DRIVE_CLK1){
                             case  2: si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA); break;
                             case  4: si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_4MA); break;
                             case  6: si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_6MA); break;
                             case  8: si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA); break;
  }

  switch (SI5351_DRIVE_CLK2){
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

void loop(){
 currentTime = millis();
//-------------------Проверка каждые 3 мс
      if(currentTime >= (loopTime + 3)) // проверяем каждые 5мс (200 Гц)
      {
            Check_enc();
              
              test();        
         
         smeter_count ++;
         loopTime = currentTime; // Счетчик прошедшего времени      
      }
//-------------------Конец проверка каждые 3 мс

    if ( (smeter_count%100 == 0) and !setup_flag) {
          
        count_avr++;
        if (tx_flag) {
            uFRD += analogRead(FRD);
            uREV += analogRead(REV);
        }
        else {
            uSMETER += analogRead(Smeter);
        }

        if (count_avr >= 10) {

          smeter_count = 0;
          count_avr = 0;

                if (tx_flag) {
                  uFRD = uFRD/10;
                  uREV = uREV/10;

                        lcd.setCursor(0,1);
                        lcd.print("P");
                        lcd.print("    ");
                        lcd.setCursor(1,1);
                        lcd.print(uFRD);
                        
                        lcd.setCursor(5,1);
                        lcd.print("F");
                        lcd.print("   ");
                        lcd.setCursor(6,1);
                        lcd.print(uREV);
        
                        lcd.setCursor(9,1);
                        lcd.print("SWR");
                        lcd.print( (float(uFRD+uREV)/(uFRD-uREV)) );
                        lcd.print("   ");
                }
                else {
                  uSMETER = uSMETER/10;
                  lcd.setCursor(0,1);
                  lcd.print(uSMETER); 
                  lcd.print("   ");
                }
        }//End  if (count_avr > 3) 
    }//End s_meter
    
    if (step_flag){

          if(enc_flag || rewrite_flag){      
                lcd.setCursor(10,1);
                      
                      step_count = step_count+enc_move;

                      if (step_count > 5){ step_count = 5;};
                      if (step_count == 0){ step_count = 1;};

	            switch (step_count){
                             case  1: STEP=10;       lcd.print("  10"); break;   //10Hz
                             case  2: STEP=100;      lcd.print(" 100"); break;   //100Hz
                             case  3: STEP=1000;     lcd.print("  1k"); break;   //1kHz
                             case  4: STEP=10000;    lcd.print(" 10k"); break;   //10kHz
                             case  5: STEP=100000;   lcd.print("100k"); break;   //100kHz
                    }
                 lcd.print("Hz");
              if (rewrite_flag){
                  step_flag = false;
                  rewrite_flag = false;
              }
              enc_flag = false;
          }
    }//End Step flag
    
    if (setup_flag){
        F_setup();
    }
    
}//End loop

void test(){

             // Проверка кнопок
              if (Button_flag == 0)
              {
               
                 if (digitalRead(Button_TX) == 0){
                      Button_flag = true;
                      F_tx();
                 }//End Button_Tx

                 if (digitalRead(Button_Step) == 0){
                      Button_flag = true;
                      step_flag = !step_flag;
                      enc_block = !enc_block;
                      if(setup_flag){enc_block = true;}
                      
                 }//End Button_Step

                 if (digitalRead(Button_VCXO) == 0){
                       Button_flag = true;                 
                       vcxo_flag = !vcxo_flag;
                       F_if2();

                 }//End Button_VCXO
                 
                 if (digitalRead(Button_RIT) == 0){
                       Button_flag = true;                 
                       rit_flag = !rit_flag;
                       menu_count = 0;

                 }//End Button_RIT

             }

             if (digitalRead(Button_TX) == 1 && digitalRead(Button_Step) == 1 && digitalRead(Button_VCXO) == 1 && digitalRead(Button_RIT) == 1 && Button_flag == true)
             {
                      Button_flag = false;
                      if ( menu_count > 0 && menu_count < 128 && setup_flag == false){
                          F_rit();
                      }
             }

             if ( digitalRead(Button_RIT) == 0 && Button_flag == true && menu_count < 255 ){
                     menu_count++;
                     if (menu_count == 254){
                         if(setup_flag){
                             lcd.clear();
                             enc_block = false;
                             F_print();
                             step_flag = 1;
                             rewrite_flag = 1;
                             setup_flag=false; 
                         }
                         else{
                             enc_flag = true;
                             F_setup();
                         }
                 }
             }
            //конец проверок кнопок

             if (enc_flag && enc_block == 0)
             {
                    if(rit_flag == false)
                    {
                          Ftx += (STEP*100)*enc_move;
                            if (Ftx < Fmin){ Ftx = Fmin; }
                            if (Ftx > Fmax){ Ftx = Fmax; }
                          Frit = Ftx;
                    }
                    else
                    {
                            Frit = Frit + (STEP*100)*enc_move;
                            if (Frit < Fmin){ Frit = Fmin; }
                            if (Frit > Fmax){ Frit = Fmax; }
                    }

                    si5351.set_freq(Frit+IF, SI5351_CLK0 );
                    enc_flag = false;
                
                  F_print();
              }
}

void F_print(){

  uint16_t mid;
                //3.110.000.00
                //9999.000.00
                mid = Frit/100000;    //3.110.
                lcd.setCursor(0,0);
                lcd.write(' ');
                                
                if ( (mid/1000) > 9)
                {
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

void Read_Value_EEPROM()
{
/*
  EEPROM
  0-3   ( 4 Byte) IF
  4-7   ( 4 Byte) XTAL
  8-11  ( 4 Byte) Fmin
  12-15 ( 4 Byte) Fmax
  16-19 ( 4 Byte) Fcorr
  20-23 ( 4 Byte) Ftx
  24-25 ( 2 Byte) Ftone
  26    ( 1 Byte) SI5351_DRIVE_CLK0   2MA 4MA 6MA 8MA 
  27    ( 1 Byte) SI5351_DRIVE_CLK1
  28    ( 1 Byte) SI5351_DRIVE_CLK2
  29    ( 1 Byte) SI5351_CAPACITOR_LOAD 0pF 6pF 8pF 10pF
  30-33 ( 4 Byte) IF2
  34    ( 1 Byte) x*F
*/
      EEPROM.get(0, IF);     //Первая ПЧ
      if (IF > 4000000000){
        IF = 0;
        EEPROM.put(0, IF);
      }
      EEPROM.get(4, XTAL);   //XTAL = 27000000;
      if (XTAL > 4000000000){
        XTAL = 27000000;
        EEPROM.put(4, XTAL);
      }
      EEPROM.get(8, Fmin);   //Fmin = 2.5кГц
      if (Fmin > 4000000000){
        Fmin = 250000;
        EEPROM.put(8, Fmin);
      }
      EEPROM.get(12, Fmax);  //Fmax = 3000000000;          // max 30 MHz  30.000.000.00
      if (Fmax > 4000000000){
        Fmax = 4000000000;
        EEPROM.put(12, Fmax);
      }
      EEPROM.get(16, Fcorr); //Fcorr = 0;
      if (Fcorr > 50000000){
          Fcorr = 50000000;
          EEPROM.put(16, Fcorr);          
      }
      if (Fcorr < -50000000){
          Fcorr = -50000000;
          EEPROM.put(16, Fcorr);
      }
      EEPROM.get(20, Ftx);   //Ftx  = 311000000;
      if (Ftx > 4000000000){
        Ftx = 300000000;
        EEPROM.put(20, Ftx);
      }
      EEPROM.get(24, Ftone); //Ftone  = 1000;
      if (Ftone > 60000){
        Ftone = 1000;
        EEPROM.put(24, Ftone);
      }
      EEPROM.get(26, SI5351_DRIVE_CLK0); // Driver current
      if (SI5351_DRIVE_CLK0 > 8){
        SI5351_DRIVE_CLK0 = 2;
        EEPROM.put(26, SI5351_DRIVE_CLK0);
      }
      EEPROM.get(27, SI5351_DRIVE_CLK1); // Driver current
      if (SI5351_DRIVE_CLK1 > 8){
        SI5351_DRIVE_CLK1 = 2;
        EEPROM.put(27, SI5351_DRIVE_CLK1);
      }
      EEPROM.get(28, SI5351_DRIVE_CLK2); // Driver current
      if (SI5351_DRIVE_CLK2 > 8){
        SI5351_DRIVE_CLK2 = 2;
        EEPROM.put(28, SI5351_DRIVE_CLK2);
      }
      EEPROM.get(29, SI5351_CAPACITOR_LOAD); // Crystal load capacitor
      if (SI5351_CAPACITOR_LOAD > 10){
        SI5351_CAPACITOR_LOAD = 0;
        EEPROM.put(29, SI5351_CAPACITOR_LOAD);
      }
      EEPROM.get(30, IF2);
      if (IF2 > 4000000000){
        IF2 = 45000000;
        EEPROM.put(30, IF2);
      }
      EEPROM.get(34, xF); //xF  = 1; multiplier F
      if (xF > 2){
        xF = 1;
        EEPROM.put(34, xF);
      }

      STEP = 1000;
      Frit = Ftx;
      
}// End Read EEPROM

void Check_enc(){

               Enc_state = PIND&B00001100;
               enc_move = 0;
               
               if( Enc_state != Enc_last ){

					switch (Enc_state){
                                                case  4: if(Enc_last == 12) enc_move=1; if(Enc_last == 8)  enc_move=-1; break;
                                                case  8: if(Enc_last == 4)  enc_move=1; if(Enc_last == 12) enc_move=-1; break;
                                                case 12: if(Enc_last == 8)  enc_move=1; if(Enc_last == 4)  enc_move=-1; break;
  					}
                  Enc_last = Enc_state;
                  enc_flag = true;
               }//End Проверка состояния encoder

}//End Check Enc

void F_tx(){

  enc_block = !enc_block;
  tx_flag = !tx_flag;
                      if (tx_flag == true){
                            si5351.output_enable(SI5351_CLK0, 0);
                            digitalWrite(TX_out, HIGH);
                            lcd.setCursor(0,1);
                            lcd.print("TX");
                            delay(100);
                            si5351.set_freq(Ftx*xF, SI5351_CLK1 );
                            si5351.output_enable(SI5351_CLK1, 1);
                      }
                      else{
                           si5351.output_enable(SI5351_CLK1, 0);
                           lcd.setCursor(0,1);
                           lcd.print("                ");
                           delay(100);
                           digitalWrite(TX_out, LOW);
                           si5351.set_freq(Frit+IF, SI5351_CLK0); //Set RX
                           si5351.output_enable(SI5351_CLK0, 1);
                           step_flag = 1;
                           rewrite_flag = 1; 
                      }
}//End F tx

void F_if2(){

                      if (vcxo_flag == true){
                            si5351.set_freq(IF2, SI5351_CLK2 );
                            si5351.output_enable(SI5351_CLK2, 1);
                            lcd.setCursor(14,0);
                            lcd.print("IF");
                      }
                      else{
                             si5351.output_enable(SI5351_CLK2, 0);
                             lcd.setCursor(13,0);
                             lcd.print("   "); 
                      }

}//End F if

void F_rit(){

                      if (rit_flag == true){
                            lcd.setCursor(10,0);
                            lcd.print("RIT");
                      }
                      else{
                              Frit = Ftx;
                              si5351.set_freq(Frit+IF, SI5351_CLK0 );
                              lcd.setCursor(10,0);
                              lcd.print("   ");
                              F_print(); 
                      }

  
}// End F rit

void F_setup(){

      if (setup_flag == false){
  
          enc_block = true;
          setup_flag = true;
  
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Setup");
          delay(400);
          lcd.clear();
      }

    if (enc_flag){
        
        if(rit_flag){
               setup_count = setup_count+enc_move;
               if (setup_count > 16){ setup_count = 16;};
               if (setup_count == 0){ setup_count = 1;};
        }
        else{
	            switch (setup_count){
                             case  1: IF+=(STEP*100)*enc_move; if(IF > 4200000000) IF = 0; if(IF > 4000000000) IF = 4000000000; break;
                             case  2: XTAL+=(STEP)*enc_move; if(XTAL > 45000000) XTAL=45000000; break;
                             case  3: Fmin+=(STEP*100)*enc_move; if(Fmin < 250000) Fmin=250000; break;
                             case  4: Fmax+=(STEP*100)*enc_move; if(Fmax > 4000000000) Fmax=4000000000; break;
                             case  5: Fcorr+=(STEP/10)*enc_move; if(Fcorr > 10000000) Fcorr=10000000; if(Fcorr < -10000000) Fcorr=-10000000;  break;
                             case  6: Ftx+=(STEP*100)*enc_move; if(Ftx > Fmax) Ftx=Fmax; if(Ftx < Fmin) Ftx=Fmin; break;
                             case  7: Ftone+=(STEP/10)*enc_move; break;
                             case  8: if(enc_move == 1){ ftone_flag = true; tone(tone_pin,Ftone); } if(enc_move == -1){ ftone_flag = false; noTone(tone_pin); }; break;
                             case  9: if(enc_move == 1) { F_eeprom_w(); lcd.setCursor(0,1); lcd.print("    Complite!!! "); delay (1000); }; break; // EEPROM write
                             case  10: if(enc_move == 1) softReset(); break; // soft reboot
                             case  11: if(enc_move == 1) xF = 2; if(enc_move == -1) xF = 1; break; // xF
                             case  12: SI5351_DRIVE_CLK0+=enc_move*2; if(SI5351_DRIVE_CLK0 < 2) SI5351_DRIVE_CLK0 = 2; if(SI5351_DRIVE_CLK0 > 8) SI5351_DRIVE_CLK0 = 8;   break;
                             case  13: SI5351_DRIVE_CLK1+=enc_move*2; if(SI5351_DRIVE_CLK1 < 2) SI5351_DRIVE_CLK1 = 2; if(SI5351_DRIVE_CLK1 > 8) SI5351_DRIVE_CLK1 = 8;   break;
                             case  14: SI5351_DRIVE_CLK2+=enc_move*2; if(SI5351_DRIVE_CLK2 < 2) SI5351_DRIVE_CLK2 = 2; if(SI5351_DRIVE_CLK2 > 8) SI5351_DRIVE_CLK2 = 8;   break;
                             case  15: SI5351_CAPACITOR_LOAD+=enc_move*2; if(SI5351_CAPACITOR_LOAD < 6) SI5351_CAPACITOR_LOAD = 0; if(SI5351_CAPACITOR_LOAD > 10) SI5351_CAPACITOR_LOAD = 10; break;
                             case  16: IF2+=(STEP*100)*enc_move; if(IF2 > 4200000000) IF2 = 0; if(IF2 > 4000000000) IF2 = 4000000000; break;
                    }
        }
                lcd.clear();
                lcd.setCursor(0,0);
	            switch (setup_count){
                             case  1:    lcd.print("IF");lcd.setCursor(0,1);lcd.print(IF/100); break;
                             case  2:    lcd.print("XTAL");lcd.setCursor(0,1);lcd.print(XTAL); break;
                             case  3:    lcd.print("Fmin");lcd.setCursor(0,1);lcd.print(Fmin/100); break;
                             case  4:    lcd.print("Fmax");lcd.setCursor(0,1);lcd.print(Fmax/100); break;
                             case  5:    lcd.print("Fcorr");lcd.setCursor(0,1);lcd.print(Fcorr); break;
                             case  6:    lcd.print("Ftx");lcd.setCursor(0,1);lcd.print(Ftx/100); break;
                             case  7:    lcd.print("Ftone");lcd.setCursor(0,1);lcd.print(Ftone); break;
                             case  8:    lcd.print("Ftone On/Off");lcd.setCursor(0,1);if( ftone_flag) lcd.print("On"); else lcd.print("Off"); break;
                             case  9:    lcd.print("EEPROM Write");lcd.setCursor(0,1);lcd.print("No/Yes?"); break;
                             case  10:   lcd.print("Reboot No/Yes?"); break;
                             case  11:   lcd.print("xF"); lcd.setCursor(0,1);lcd.print(xF);break;
                             case  12:   lcd.print("DRIVE_CLK0"); lcd.setCursor(0,1);lcd.print(SI5351_DRIVE_CLK0);break;
                             case  13:   lcd.print("DRIVE_CLK1"); lcd.setCursor(0,1);lcd.print(SI5351_DRIVE_CLK1);break;
                             case  14:   lcd.print("DRIVE_CLK2"); lcd.setCursor(0,1);lcd.print(SI5351_DRIVE_CLK2);break;
                             case  15:   lcd.print("CAPACITOR_LOAD"); lcd.setCursor(0,1);lcd.print(SI5351_CAPACITOR_LOAD);break;
                             case  16:   lcd.print("IF2");lcd.setCursor(0,1);lcd.print(IF2/100); break;
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
      EEPROM.get(4, temp);   //XTAL = 27000000;
      if (XTAL != temp){
        EEPROM.put(4, XTAL);
      }
      EEPROM.get(8, temp);   //Fmin = 30000000;            // min 300kHz   
      if (Fmin != temp){
        EEPROM.put(8, Fmin);
      }
      EEPROM.get(12, temp);  //Fmax = 3000000000;          // max 30 MHz  30.000.000.00
      if (Fmax != temp){
        EEPROM.put(12, Fmax);
      }
      
      EEPROM.get(16, temp_l); //Fcorr = 0;
      if (Fcorr != temp_l){
        EEPROM.put(16, Fcorr);
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
      
      temp = SI5351_CAPACITOR_LOAD;      
      EEPROM.get(29, SI5351_CAPACITOR_LOAD); // Crystal load capacitor
      if (SI5351_CAPACITOR_LOAD != temp){
        SI5351_CAPACITOR_LOAD = temp;
        EEPROM.put(29, SI5351_CAPACITOR_LOAD);
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


}

//----------------------------
void softReset(){

  asm volatile ("  jmp 0");

}//End soft reset

