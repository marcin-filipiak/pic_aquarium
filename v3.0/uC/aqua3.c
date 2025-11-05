#include <aqua3.h>

#use i2c(master,sda=PIN_B0,scl=PIN_B1)

//////////////definicje stalych///////////////////////////////////

#define E_UP   0
#define E_DOWN 1
#define E_OK   2
#define E_IDLE 3

//domyslne ustawienia kolejnych bitow w pamieci EEPROM zaraz po zaprogramowaniu
//#if defined (__PCM__) 
//#rom int8 0x2100={26,0,0,0,0,0,0,0,0} 
//#elif defined(__PCH__) 
//#rom int 0xf00000={} 
//#endif 

#define EEPROM_TEMP            0x00 //temperatura
#define EEPROM_LIGHT_HOUR_ON   0x1  //godzina wlaczenia swiatla
#define EEPROM_LIGHT_MIN_ON    0x2  //minuta wlaczenia swiatla

#define EEPROM_LIGHT_HOUR_OFF  0x3  //godzina wylaczenia swiatla
#define EEPROM_LIGHT_MIN_OFF   0x4  //minuta wylaczenia swiatla

#define EEPROM_FEED_HOUR_CYCLE 0x5 //cykl co ile godzin wlaczyc karmienie
#define EEPROM_FEED_SEC_DELAY  0x6 //czas wlaczenia karmienia

#define EEPROM_AIR_HOUR_CYCLE 0x7 //cykl co ile godzin wlaczyc powietrze
#define EEPROM_AIR_MIN_DELAY  0x8 //czas wlaczenia powietrza

/////////////definicje pinow//////////////////////////////////////

#define P_FEED          PIN_C3 //karmienie
#define P_AIR           PIN_D0 //napowietrzanie
#define P_LIGHT         PIN_C1 //oswietlenie triak
#define P_HEATER        PIN_C2 //grzalka triak
#define P_BEEPER        PIN_D1 //sygnalizator dzwiekowy
#define P_LCDLIGHT      PIN_D2 //podswietlenie ekranu
#define DQ              PIN_B3 //dane 1wire
#define DS1820_DATAPIN PIN_B3

/////////////definicje stanow/////////////////////////////////////

#define OFF output_low 
#define ON  output_high 

//////////////deklaracja zmiennych///////////////////////////////

char menu_event=E_IDLE;
char menu_pos=0;
boolean menu_select=false;
//byl byte zmienione na char
char sec,min,hour;
char LIGHT_HOUR_ON, LIGHT_HOUR_OFF;

int8 last_air_hour_on = 0; //ostatnie wlaczenie napowietrzania
int8 last_feed_hour_on = 0; //ostatnie wlaczenie karmienia

int8 numDev = 0; 
float now_t_aqua, set_t;

/////////////zewnetrzne pliki////////////////////////////////////

#include "drv\lcd.c"             //obsluga wyswietlacza
#include "drv\ds1307.c"     //zegar czasu rzeczywistego
#include "conversion.c"      //konwersje liczbowe
#include "drv\ds1820.c"     //termometry na 1wire
#include "functions.c"
#include "menu.c"

//////////////Interrupt procedure/////////////////////////////////
#INT_RTCC
clock_isr() { 
   restart_wdt();
}

//////////////main program starts here//////////////////////////
void main() {
   
   int16 started_air = 0;   //w sekundach startu powietrza
   int16 started_feed = 0;  //w sekundach startu karmienia

   ON(P_LIGHT);
   ON(P_HEATER);
   ON(P_LCDLIGHT);      
   
   delay_ms(100);

  //ustawienie watchdoga
  setup_wdt(WDT_2304MS);

  //ustawienie timera ktory zglasza sie watchdogowi
   setup_timer_0( RTCC_INTERNAL | RTCC_DIV_256 );
   set_timer0(0);
   enable_interrupts(INT_RTCC);
   enable_interrupts(GLOBAL);
   

   //ustawienia wejsc analogowych
   //setup_adc( ADC_CLOCK_INTERNAL );
   setup_adc_ports(AN0_AN1_AN2_AN3_AN4); 
   setup_adc(ADC_CLOCK_DIV_64);

/*
   setup_ccp2(CCP_PWM);   // Configure CCP2 as a PWM 
   int16 mm=200;
   long duty=10;
   setup_timer_2(T2_DIV_BY_16, mm, 1); 
   set_pwm2_duty(duty); 
*/       
        
   //inicjalizacja wyswietlacza
   delay_ms(700);
   lcd_init();
  delay_ms(700);
   //wyslanie do pamieci ekranu wlasnych znakow
   lcd_init_custom_chars();
   //wyczyszczenie ekranu
   delay_ms(300);
   lcd_putc("\fNoweEnergie.org\nv3.4");  
  delay_ms(900);

  //jesli ustawienia sa czyste (swiezo po zaprogramowaniu) wczytanie domyslnych danych
  if( read_eeprom(0x00) == 255){
       lcd_putc("\fWczytanie\ndomyslnych ustawien");  
       default_set();
       delay_ms(900);
   }   

 /*
   int8 anr =0;
   delay_ms(1000);
   set_adc_channel(1);
   delay_ms(1);
   anr=read_ADC();
   if(anr < 10){
           printf(lcd_putc,"\fTryb testowy");
           while(1){
                   set_adc_channel(1);
                   delay_ms(1);
                   anr=read_ADC();
                   printf(lcd_putc,"\f A/D: %u",anr);
                   delay_ms(1000);
           }
   }
*/   
     
// Find devices on the bus 
numDev = 0;
if ( DS1820_FindFirstDevice() ){
 do
    {
        //now_t_aqua = DS1820_GetTempFloat();
       numDev ++;
    }
    while ( DS1820_FindNextDevice() );
}
   if (numDev>0){
           printf(lcd_putc,"\fTermometr\nzainstalowany."); 
   }
   else {
          printf(lcd_putc,"\fPodlacz\ntermometr!"); 
   }
          
   delay_ms(1000);   
   printf(lcd_putc,"\f");


//wczytanie zapisanych ustawien z eeprom
LIGHT_HOUR_ON = read_eeprom(EEPROM_LIGHT_HOUR_ON);
LIGHT_HOUR_OFF = read_eeprom(EEPROM_LIGHT_HOUR_OFF);
//LIGHT_MIN_ON = read_eeprom(EEPROM_LIGHT_MIN_ON);
//LIGHT_MIN_OFF = read_eeprom(EEPROM_LIGHT_MIN_OFF);
   
   while(true){
        
         
        
        //--klik w jakis button, wejscie w menu--
        //UWAGA: obsluga menu musi byc na poczatku petli, 
        //bo inaczej np. odczyty czasu by byly przedatowane 
        //(nie sa odczytywane przez czas pobytu w menu)
        
        //menu_event = ReadResistometrMenu(ana);
        menu_event = getButton();
        
        //jesli poruszamy sie po menu
        if (menu_event != E_IDLE) {
                ON(P_LCDLIGHT); //zapal podswietlenie LCD
                change_menu();  //obsluga menu
                delay_ms(300);
        }
        
        
        //ODCZYT CZASU Z RTC
        read_time();
        delay_ms(50);
        
        //ODCZYT TEMPERATUR
        //termometr wody
        if (numDev>0){
               if ( DS1820_FindFirstDevice() ){
               do
                {
                 now_t_aqua = DS1820_GetTempFloat();
                 }
                while ( DS1820_FindNextDevice() );
                delay_ms(50);
                }
        }
        
      
        //ODCZYT NASTAW TEMPERATUR
        set_t = read_eeprom(0x00);
        //jesli nie jest ustawiona to napewno nie jest 255stopni ustawienie domyslnej 25stopni
        if (set_t == 255) set_t=25;
                
        //---wyswietlenie biezacych informacji i wykonywanie biezacych zadan---
        if (menu_pos == 0){

            //wyswietlenie glownego ekranu kontrolnego
            print_desktop();
                               
            //PODSWIETLANIE EKRANU
            //od godziny 17:00 do 8:00
            if ( hour > 17 || hour < 8){
                     ON(P_LCDLIGHT); //zapal podswietlenie LCD
             }
             //w dzien wylaczony
             else {
                     OFF(P_LCDLIGHT); //wylacz podswietlenie LCD
             }
                    
            //GRZALKA
            //jesli nie ma bledu odczytu
           //BYLO: if (now_t_aqua != 1111.0 && numDev>0){
           if (now_t_aqua < 150 && numDev>0){
               //jesli nastawiona jest wieksza od obecnej to wlacz grzalke
               if (set_t > now_t_aqua){
                        ON(P_HEATER);
               }
               //jesli jest za wysoka to wylacz grzalke
               if (set_t < now_t_aqua) {
                        OFF(P_HEATER);
               }
            }
            //jesli jest blad odczytu
            else {
                        OFF(P_HEATER);
            }

            
            //INSTRUKCJE WYWOLANE ZEGAREM
            //TODO: obawiam sie ze nie dziala np. ON18:00 OFF 3:00
            //jesli czas wlaczenia oswietlenia
            /*
             if LIGHT_HOUR_ON <= hour){
                  if (LIGHT_MIN_ON) <= min){
                     ON(P_LIGHT);
                    }
             }
             //jesli czas wylaczenia oswietlenia
             if (LIGHT_HOUR_OFF <= hour){ 
                   if(LIGHT_MIN_OFF <= min){
                       OFF(P_LIGHT);
                    }
             }*/
             if ( ( hour >= LIGHT_HOUR_ON ) && ( hour < LIGHT_HOUR_OFF )){
                       ON(P_LIGHT);
             }
             else {
                       OFF(P_LIGHT);
              }
             
             //jesli czas karmienia i oczywiscie jesli jest wlaczony w opcjach
             if (read_eeprom(EEPROM_FEED_HOUR_CYCLE) != 0){
                if (read_eeprom(EEPROM_FEED_HOUR_CYCLE) <= (hour - last_feed_hour_on)){
                        ON(P_FEED);                                        
                        //nie byl wystartowany to wystartowac wpisujac czas startu
                        if (started_feed == 0){
                                //wpisac czas w sekundach kiedy wystartowal
                                started_feed = ((hour * 3600) + (min * 60) + sec);
                        }
                        //zostal wystartowany
                        else {
                                //obliczenie roznicy sekund miedzy startem a teraz
                                int16 abso =  ((hour * 3600) + (min * 60) + sec) - started_feed;                                
                                //czas zakonczony
                                if ( read_eeprom(EEPROM_FEED_SEC_DELAY) < (int8)abso){
                                        OFF(P_FEED);   
                                        started_feed = 0; 
                                        last_feed_hour_on = hour;
                                }
                        }


                }
             }
             
             //jesli czas wlaczenia napowietrzania i oczywiscie jesli jest wlaczony w opcjach
             if (read_eeprom(EEPROM_AIR_HOUR_CYCLE) != 0){
                if (read_eeprom(EEPROM_AIR_HOUR_CYCLE) <= (hour - last_air_hour_on)){
                        ON(P_AIR);                                        
                        //nie byl wystartowany to wystartowac wpisujac czas startu
                        if (started_air == 0){
                                //wpisac czas w minutach kiedy wystartowal
                                started_air = ((hour * 60) + min);
                        }
                        //zostal wystartowany
                        else {
                                //obliczenie roznicy minut miedzy startem a teraz
                                int16 abso =  ((hour * 60) + min) - started_air;                                       
                                //czas zakonczony
                                if ( read_eeprom(EEPROM_AIR_MIN_DELAY) < (int8)abso){
                                        OFF(P_AIR);   
                                        started_air = 0; 
                                        last_air_hour_on = hour;
                                }
                        }
                
                }
             }
             
             
       }
   }

}