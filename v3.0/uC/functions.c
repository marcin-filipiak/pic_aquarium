/*
 sygnal dzwiekowy

*/
void beep(int l=10){
      ON(P_BEEPER);
      delay_ms(l);
      OFF(P_BEEPER);
      delay_ms(5);
}

/*
  sprawdzenie jaki przycisk z panelu sterowania jest klik
  domyslny stan pinow na rezystorze podciagajacym:
  E_OK   (E2) - false
  E_DOWN (C0) - true
  E_UP   (C1) - true
  po ich wcisnieciu stan przeciwny do domyslnego, 
  czyli aktywacja sygnalem odwrotnym do domyslnego.
*/
char enterimp = 4;  

int getButton() {
    
    set_adc_channel(1);
    delay_ms(1);
    int8 an=read_ADC();
    delay_ms(3);

    //0
    if (an < 10){
            enterimp = 0;
            //printf(lcd_putc,"\f down ");
            delay_ms(500);
	    beep();
            return E_DOWN;
    } 
    //246
    if (an > 230 && an < 255){
               if (enterimp==2){
                        enterimp = enterimp + 1;
                        //printf(lcd_putc," ok ");
                        delay_ms(500);
			beep();
                        return E_OK;        
                }
                if (enterimp < 3){
                        enterimp = enterimp + 1;
                        //printf(lcd_putc," idle ");
                        delay_ms(500);
                        return E_IDLE;
                }
             return E_IDLE;

    }
    //156
    if (an > 140 && an < 165){
             beep();
             enterimp = 0;
             //printf(lcd_putc,"\f up ");
             delay_ms(500);
             return E_UP;
    }
    
    return E_IDLE;
    
}

void print_aqua_temp(float t){
   if (t!=111.0){
      lcd_gotoxy(1,2);
      /*
      sprintf(ctemp, "%3.1f", ds1820_read());
      for (x=0; x<4; x++){ 
            lcd_putc(ctemp[x]);
      }*/
      lcd_send_byte(1,0x4); //symbol wody
      printf(lcd_putc, "%3.1f", t); 
      lcd_send_byte(1,0x2); //stopnie celsjusza
   }
   else {
      lcd_gotoxy(1,2);
      printf(lcd_putc, "      "); 
   }
   
   
}



/*
wyswietlenie na ekranie 
informacji o temperaturze
w pomieszczeniu
/*
void print_room_temp(float t){
   lcd_gotoxy(11,2);
   
   //sprintf(ctemp, "%3.1f", ds1820_read());
   //for (x=0; x<4; x++){ 
   //        lcd_putc(ctemp[x]);
   //}
   lcd_send_byte(1,0x5); //symbol domku
   printf(lcd_putc, "%3.1f", t); 
   lcd_send_byte(1,0x2); //stopnie celsjusza
   
   
}*/

void print_time(){
  lcd_gotoxy(8,2);
  lcd_send_byte(1,0x6); //symbol zegarka
  printf(lcd_putc, "%02d:%02d:%02d", hour,min,sec);
}







//czytanie zegara
void read_time()
{
   sec = bcdToDec(read_ds1307 (0)); // read second
   delay_ms(1);
   min = bcdToDec(read_ds1307 (1)); // read minute
   delay_ms(1);
   hour = bcdToDec(read_ds1307 (2)); // read hour
   delay_ms(1);
   //day = bcdToDec(read_ds1307 (4)); // read day
   //month = bcdToDec(read_ds1307 (5)); // read month
   //year = bcdToDec(read_ds1307 (6)); // read year
}

//ustawianie zegara
void set_time()
{
   write_ds1307 (4, decToBcd(1)); //day
   write_ds1307 (5, decToBcd(1)); //month
   write_ds1307 (6, decToBcd(0)); //year
   write_ds1307 (2, decToBcd(hour)); //hour;
   write_ds1307 (1, decToBcd(min)); //minute
   write_ds1307 (0, decToBcd(0)); //second

}


/*
 dodaje do hour:min:sec czas podany w sekundach
 i zwraca w tablicy w formacie hour:min:sec
 */
void sum_during(byte hour, byte min, byte sec, byte during, byte t[]){
    //przeliczenie podanego czasu na sekundy
    //int as = (hour * 3600) + (min * 60) + sec;
    int16 as = (hour * 3600) + (min * 60) + sec;
    
    int16 d;
    //int t[2]; //h:m:s
    //dodanie do podanego czasu czasu trwania (during)
    d = as + during;
    //przeliczenie na format czasu
    t[0] =  (byte)(((d / 60) / 60) % 24);
    t[1] =  (byte)((d / 60) % 60);
    t[2] =  (byte)(d % 60);
    printf(lcd_putc,"\f%lu %lu\n",as, d);
    delay_ms(2000);
}


//domyslne ustawienia do pamieci
void default_set(){
	write_eeprom(EEPROM_TEMP,26);
	write_eeprom(EEPROM_LIGHT_HOUR_ON,7);
	write_eeprom(EEPROM_LIGHT_MIN_ON,1);
	write_eeprom(EEPROM_LIGHT_HOUR_OFF,18);
	write_eeprom(EEPROM_LIGHT_MIN_OFF,1);
	write_eeprom(EEPROM_FEED_HOUR_CYCLE,0);
	write_eeprom(EEPROM_FEED_SEC_DELAY,0);
	write_eeprom(EEPROM_AIR_HOUR_CYCLE,0);
	write_eeprom(EEPROM_AIR_MIN_DELAY,0);
}

//////////////////////////////////////////////////////////////////////
void print_desktop(){

	printf(lcd_putc,"\f");
        //aktywna grzalka
	if (input_state(P_HEATER)){
			lcd_gotoxy(1,1);
                        lcd_send_byte(1,0x1);
	}
        //aktywne oswietlenie
	if (input_state(P_LIGHT)){
                     lcd_gotoxy(2,1);
                     lcd_send_byte(1,0x0);
	}
        //aktywne karmienie
	if (input_state(P_FEED)){
                        lcd_gotoxy(3,1);
                        lcd_send_byte(1,0x5);
	}
	//aktywne napowietrzanie           
	if (input_state(P_AIR)){
                        lcd_gotoxy(4,1);
                        lcd_send_byte(1,0x7);
	}

	//wyswietl tmperature wody
        if (numDev>0){
               print_aqua_temp(now_t_aqua);
        }

        //ALARM ZBYT WYSOKIEJ TEMPERATURY
        if (numDev>0){
                   if(now_t_aqua >= 29){
                       lcd_gotoxy(5,1);
                       lcd_send_byte(1,0x3);
                    }
                    if(now_t_aqua >= 29 && sec==0){
                       beep(500);
                    }
        }

	print_time();
}

