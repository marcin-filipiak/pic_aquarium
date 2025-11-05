/*
wyswietlenie na ekranie
funkcji klawiszy przewijania menu
*/
void print_menubar(){
   lcd_gotoxy(1,2);
   lcd_putc("<"); //strzalka w lewo
   lcd_gotoxy(8,2);
   //lcd_putc((char)126); //enter
   lcd_putc("ok");
   lcd_gotoxy(16,2);
   lcd_putc(">"); //strzalka w prawo
}


////////////////////////////////////////////



/*
ustawienie w menu temperatury
*/
void SetTemperature(){
    boolean run=true;
    int8 t;

    t = read_eeprom(0x00);
    //jesli nie jest ustawiona to napewno nie jest 255 stopni ustawienie domyslnej 25 stopni
    if (t == 255) t=25;

    lcd_putc("\f");
    print_menubar();
    delay_ms(800);
    while(run){
        delay_ms(800);
        lcd_gotoxy(2,1);
        printf(lcd_putc, "Utrzymuj %u ", t); 
        lcd_send_byte(1,0x2); //stopnie celsjusza
        
        switch(getButton()){
            case E_DOWN:
                t = t-1;
                break;
            case E_UP:
                t = t+1;
                break;
            case E_OK:
                write_eeprom(0x00,t);
                run=false;
        }
    }

}

/*
ustawianie godziny
*/
int8 menu_hour(int8 h)
{
   boolean run=true;
   lcd_putc("\f");
   print_menubar();
   delay_ms(800);
   //ustawianie godziny
   while(run){
        //delay_ms(800);
        lcd_gotoxy(2,1);
        printf(lcd_putc, "Godzina %02d  ", h); 
        
        switch(getButton()){
            case E_DOWN:
                h = h-1;
                if(h<0 || h==255) h=24;
                break;
            case E_UP:
                h = h+1;
                if(h>24) h=0;
                break;
            case E_OK:                
                run=false;
        }
    }
    return h;
}

/*
ustawianie minut
*/
int8 menu_minute(int8 m)
{
   boolean run=true;
   lcd_putc("\f");
   print_menubar();
   delay_ms(800);
   while(run){
        //delay_ms(800);
        lcd_gotoxy(2,1);
        printf(lcd_putc, "Minuta %02d  ", m); 
        
        switch(getButton()){
            case E_DOWN:
                m = m-1;
                if(m<0 || m==255) m=59;
                break;
            case E_UP:
                m = m+1;
                if(m>59) m=0;
                break;
            case E_OK:                
                run=false;
        }
    }
    return m;
}

/*
ustawianie sekund
*/
int8 menu_sec(int8 s)
{
   boolean run=true;
   lcd_putc("\f");
   print_menubar();
   delay_ms(800);
   while(run){
        //delay_ms(800);
        lcd_gotoxy(2,1);
        printf(lcd_putc, "Sekund %02d  ", s); 
        
        switch(getButton()){
            case E_DOWN:
                s = s-1;
                if(s<0 || s==255) s=59;
                break;
            case E_UP:
                s = s+1;
                if(s>59) s=0;
                break;
            case E_OK:                
                run=false;
        }
    }
    return s;
}

///////////////////////////////////////////


/*
obsluga menu
*/
void change_menu(){

    #define MENU_ELEMENTS 7
    
    //przesuniecie sie po menu w gore
    if (menu_event == E_UP && menu_pos < MENU_ELEMENTS && menu_select==false){ menu_pos++;}
    //przesuniecie sie po menu w dol
    if (menu_event == E_DOWN && menu_select==false){
	if (menu_pos > 0){
	 menu_pos--;
	}
	if (menu_pos == 0){
		delay_ms(800);
		menu_event == E_IDLE;
		enterimp=4;
	}
    }
    //zatwierdzenie opcji w menu
    if (menu_event == E_OK && menu_pos!=0) menu_select = true;

    switch(menu_pos){
        case 0:
            lcd_putc("\fOpusc menu");
            print_menubar();
            delay_ms(1000);
            menu_select=false;
            menu_pos=0;
            lcd_putc("\f");
            break;
        case 1:
            lcd_putc("\fTemperatura");
            print_menubar();
            if (menu_select) {
                SetTemperature();
                beep();
                beep();
                menu_select=false;
                menu_pos=0;
                lcd_putc("\f");
            }
            break;
        case 2:
            lcd_putc("\fWl.oswietlenia");
            print_menubar();
            if (menu_select) {
                write_eeprom(EEPROM_LIGHT_HOUR_ON,menu_hour(read_eeprom(EEPROM_LIGHT_HOUR_ON)));
                //write_eeprom(EEPROM_LIGHT_MIN_ON,menu_minute(read_eeprom(EEPROM_LIGHT_MIN_ON)));
                //printf(lcd_putc, "\fUstawiono %02d:%02d", read_eeprom(EEPROM_LIGHT_HOUR_ON), read_eeprom(EEPROM_LIGHT_MIN_ON));
		LIGHT_HOUR_ON = read_eeprom(EEPROM_LIGHT_HOUR_ON);
		printf(lcd_putc, "\fUstawiono %02d:00", LIGHT_HOUR_ON);
                delay_ms(2000);
                beep();
                beep();
                menu_select=false;
                menu_pos=0;
                lcd_putc("\f");
            }
            break;            
        case 3:
            lcd_putc("\fWyl.oswietlenia");
            print_menubar();
            if (menu_select) {
                write_eeprom(EEPROM_LIGHT_HOUR_OFF,menu_hour(read_eeprom(EEPROM_LIGHT_HOUR_OFF)));
                //write_eeprom(EEPROM_LIGHT_MIN_OFF,menu_minute(read_eeprom(EEPROM_LIGHT_MIN_OFF)));
                //printf(lcd_putc, "\fUstawiono %02d:%02d", read_eeprom(EEPROM_LIGHT_HOUR_OFF), read_eeprom(EEPROM_LIGHT_MIN_OFF));
		LIGHT_HOUR_OFF = read_eeprom(EEPROM_LIGHT_HOUR_OFF);
		printf(lcd_putc, "\fUstawiono %02d:00", LIGHT_HOUR_OFF);
                delay_ms(2000);
                beep();
                beep();
                menu_select=false;
                menu_pos=0;
                lcd_putc("\f");
            }
            break;
        case 4:
            lcd_putc("\fNapowietrzanie");
            print_menubar();
            if (menu_select) {
                last_air_hour_on = 0;
                write_eeprom(EEPROM_AIR_HOUR_CYCLE,menu_hour(read_eeprom(EEPROM_AIR_HOUR_CYCLE)));
                write_eeprom(EEPROM_AIR_MIN_DELAY,menu_minute(read_eeprom(EEPROM_AIR_MIN_DELAY)));
                printf(lcd_putc, "\fUruchomienie\nco %02d h/%02d min", read_eeprom(EEPROM_AIR_HOUR_CYCLE), read_eeprom(EEPROM_AIR_MIN_DELAY));
                delay_ms(2000);
                beep();
                beep();
                menu_select=false;
                menu_pos=0;
                lcd_putc("\f");
            }
            break;
        case 5:
            lcd_putc("\fKarmienie");
            print_menubar();
            if (menu_select) {
                last_feed_hour_on = 0;
                write_eeprom(EEPROM_FEED_HOUR_CYCLE,menu_hour(read_eeprom(EEPROM_FEED_HOUR_CYCLE)));
                write_eeprom(EEPROM_FEED_SEC_DELAY,menu_sec(read_eeprom(EEPROM_FEED_SEC_DELAY)));
                printf(lcd_putc, "\fUruchomienie\nco %02d h/%02d s", read_eeprom(EEPROM_FEED_HOUR_CYCLE), read_eeprom(EEPROM_FEED_SEC_DELAY));
                delay_ms(2000);
                beep();
                beep();
                menu_select=false;
                menu_pos=0;
                lcd_putc("\f");
            }
            break;
        case 6:
            lcd_putc("\fZegar");
            print_menubar();
            if (menu_select) {
                hour = menu_hour(hour);
                min = menu_minute(min);
                printf(lcd_putc, "\fUstawiono %02d:%02d", hour,min);
                //zapis
                set_time();
                delay_ms(3000);
                beep();
                beep();
                menu_select=false;
                menu_pos=0;
                lcd_putc("\f");
            }
            break; 
         case 7:
            lcd_putc("\fTestuj");
            print_menubar();
            if (menu_select) {
                beep();
                beep();

                   lcd_putc("\ftest:");

                   //lampa
                   lcd_gotoxy(6,1);
                   lcd_send_byte(1,0x0);
                   ON(P_LIGHT);
                   delay_ms(2000);
                   OFF(P_LIGHT);
                   delay_ms(3000);
                   
                   //grzalka
                   lcd_gotoxy(6,1);
                   lcd_send_byte(1,0x1);
                   ON(P_LIGHT);
                   delay_ms(2000);
                   OFF(P_LIGHT);
                   delay_ms(3000);
                
                   //karmienie
                   lcd_gotoxy(6,1);
                   lcd_send_byte(1,0x5);
                   ON(P_FEED);
                   delay_ms(2000);
                   OFF(P_FEED);
                   delay_ms(3000);

                   //napowietrzanie
                   lcd_gotoxy(6,1);
                   lcd_send_byte(1,0x7);
                   ON(P_AIR);
                   delay_ms(2000);
                   OFF(P_AIR);
                   delay_ms(3000);
                
                menu_select=false;
                menu_pos=0;
                beep();
                lcd_putc("\f");
            }
            break;
            
    }
}



