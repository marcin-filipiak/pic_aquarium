
int8 RomBytes[8];      
int8 lastDiscrep = 0; 
short doneFlag = 0; 
int8 FoundROM[3][8];    // Table of found ROM codes, 8 bytes for each 
int8 numROMs; 
int8 dowcrc;            // crc is accumulated in this variable 

// crc lookup table 
int8 const dscrc_table[] = { 
   0,94,188,226,97,63,221,131,194,156,126,32,163,253,31,65, 
   157,195,33,127,252,162,64,30,95,1,227,189,62,96,130,220, 
   35,125,159,193,66,28,254,160,225,191,93,3,128,222,60,98, 
   190,224,2,92,223,129,99,61,124,34,192,158,29,67,161,255, 
   70,24,250,164,39,121,155,197,132,218,56,102,229,187,89,7, 
   219,133,103,57,186,228,6,88,25,71,165,251,120,38,196,154, 
   101,59,217,135,4,90,184,230,167,249,27,69,198,152,122,36, 
   248,166,68,26,153,199,37,123,58,100,134,216,91,5,231,185, 
   140,210,48,110,237,179,81,15,78,16,242,172,47,113,147,205, 
   17,79,173,243,112,46,204,146,211,141,111,49,178,236,14,80, 
   175,241,19,77,206,144,114,44,109,51,209,143,12,82,176,238, 
   50,108,142,208,83,13,239,177,240,174,76,18,145,207,45,115, 
   202,148,118,40,171,245,23,73,8,86,180,234,105,55,213,139, 
   87,9,235,181,54,104,138,212,149,203,41,119,244,170,72,22, 
   233,183,85,11,136,214,52,106,43,117,151,201,74,20,246,168, 
   116,42,200,150,21,75,169,247,182,232,10,84,215,137,107,53 
}; 

#define DS1820_CONN_ERROR 1111.0 // error sign 

// Returns 0 for one wire device presence, 1 for none 
int8 ow_reset(void) 
{ 
   int8 presence; 

   output_low(DQ); 
   delay_us(588);          // Min. 480uS 
   output_float(DQ); 
   delay_us(82);           // Takes 15 to 60uS for devices to respond 
   presence = input(DQ); 
   delay_us(424);          // Wait for end of timeslot 
   return(presence); 
} 
//****************************************************************************** 
//****************************************************************************** 
// Read bit on one wire bus 
int8 read_bit(void) 
{ 
   output_low(DQ); 
   delay_us(14);         // Added, 1uS min.(@4Mhz 1-2@20Mhz 3-9) 
   output_float(DQ); 
   delay_us(14);        // Read within 15uS from start of time slot (@4Mhz 6-11 @20Mhz 15-22) 
   return(input(DQ));    
}    

//****************************************************************************** 
void write_bit(int8 bitval) 
{ 
   output_low(DQ); 

   if(bitval == 1) { 
      delay_us(14);      // 1uS min. (@4Mhz 1-2 @20Mhz 3-9) 
      output_float(DQ); 
   } 
   delay_us(145);        
   output_float(DQ); 
} 

//****************************************************************************** 
int8 read_byte(void) 
{ 
   int8 i; 
   int8 val = 0; 

   for(i=0;i<8;i++) 
   { 
      if(read_bit()) val |= (0x01 << i); 
      delay_us(150);  // To finish time slot 
   } 

   return val; 
} 
//****************************************************************************** 
void write_byte(int8 val) 
{ 
   int8 i; 
   int8 temp; 

   for (i=0;i<8;i++) 
   { 
      temp = val >> i; 
      temp &= 0x01; 
      write_bit(temp); 
      delay_us(15); 
   } 

   delay_us(125); 
} 
//****************************************************************************** 
// One wire crc 
int8 ow_crc(int8 x) 
{ 
   dowcrc = dscrc_table[dowcrc^x]; 
   return dowcrc; 
} 
//****************************************************************************** 
// Searches for the next device on the one wire bus. If there are no more 
// devices on the bus then false is returned. 
int8 Next(void) 
{ 
   int8 m = 1;             // ROM Bit index 
   int8 n = 0;             // ROM Byte index 
   int8 k = 1;             // Bit mask 
   int8 x = 0; 
   int8 discrepMarker = 0; 
   int8 g;                 // Output bit 
   int8 nxt;               // Return value 
   short flag; 


   nxt = FALSE;            // Reset next flag to false 

   dowcrc = 0;             // Reset the dowcrc 

   flag = ow_reset(); 

   if (flag||doneFlag)     // If no parts return false 
   { 
      lastDiscrep = 0;     // Reset the search 
      return FALSE; 
   } 

   write_byte(0xF0);       // Send SearchROM command 

   do 
   { 
      x = 0; 
      if (read_bit() == 1) x = 2; 
      delay_us(250); 
      if (read_bit() == 1) x |= 1;   // And it's complement 

      if (x == 3)                   // There are no devices on the one wire bus 
      break; 
      else 
      { 
         if (x > 0)                 // All devices coupled have 0 or 1 
            g = x >> 1;             // Bit write value for search 

         // If this discrepancy is before the last discrepancy on a previous 
         // Next then pick the same as last time. 
         else 
         { 
            if (m < lastDiscrep) 
               g = ((RomBytes[n] & k) > 0); 
            // If equal to last pick 1 
            else 
               g = (m == lastDiscrep);  // If not then pick 0 

               // If 0 was picked then record position with mask k 
               if (g == 0) discrepMarker = m; 
         } 

         // Isolate bit in ROM[n] with mask k 
         if (g == 1) RomBytes[n] |= k; 
         else RomBytes[n] &= ~k; 

         write_bit(g);  // ROM search write 

         m++;           // Increment bit counter m 
         k = k << 1;    // and shift the bit mask k 
         // If the mask is 0 then go to new ROM 
         if (k == 0) 
         {  // Byte n and reset mask 
            ow_crc(RomBytes[n]);      // Accumulate the crc 
            n++; 
            k++; 
         } 
      } 
   } while (n < 8);  // Loop through until through all ROM bytes 0-7 

   if (m < (65||dowcrc))   // If search was unsuccessful then 
      lastDiscrep = 0;     // reset the last Discrepancy to zero 

   else  // Search was successful, so set lastDiscrep, lastOne, nxt 
   { 
      lastDiscrep = discrepMarker; 
      doneFlag = (lastDiscrep == 0); 
      nxt = TRUE; // Indicates search not yet complete, more parts remain 
   } 

   return nxt; 
} 
//****************************************************************************** 
// Resets current state of a ROM search and calls Next to find the first device 
// on the one wire bus. 
int8 First(void) 
{ 
   lastDiscrep = 0; 
   doneFlag = FALSE; 
   return Next();    // Call Next and return it's return value; 
} 
//****************************************************************************** 
int8 FindDevices(void) 
{ 
   int8 m, cont = 0; 
   if(!ow_reset()) 
   { 
      if(First())    // Begins when at least one part found 
      { 
         numROMs = 0; 
         do 
         { 
            numROMs++;  
            cont++; 
            for (m=0;m<8;m++) 
            { 
               FoundROM[numROMs][m] = RomBytes[m]; 
            } 
         } while (Next());   // Continues until no additional                              
      } 
   } 
   return cont; 
} 
//****************************************************************************** 
// Sends Match ROM command to bus then device address 
int8 Send_MatchRom(void) 
{ 
   int8 i; 
   if (ow_reset()) return FALSE;          // 0 if device present 
   write_byte(0x55);                      // Match ROM 

   for (i=0;i<8;i++) 
   { 
      write_byte(FoundRom[numROMs][i]);   // Send ROM code 
   } 

   return TRUE; 
} 

/*
float ds1820_read() 
{ 
   signed int16 temperature; 
   int8 i, scratch[9], try=3;        

   while(try--) 
   { 
      output_float(DQ);       // Set as input. 4k7 pullup on bus. 
      
      ow_reset(); 
      write_byte(0xCC); // Skip Rom command 
      write_byte(0x44); // Temperature Convert command 
      output_float(DQ); 
      delay_ms(780);    // Max. time for conversion is 750mS 
      ow_reset(); 
      write_byte(0xCC); // Skip Rom command 
      write_byte(0xBE); // Read scratch pad command 
    
      dowcrc = 0;        
    
      // Get the data bytes 
      for (i=0;i<=7;i++) 
      { 
          scratch[i] = read_byte(); 
          ow_crc(scratch[i]);          
      } 
    
      scratch[8] = read_byte();   // Get crc byte 
      ow_reset(); 
      
      // If the received crc is same as calculated then data is valid. 
      // Scale and round to nearest degree C. 
      // Scaling is 0.0625 (1/16) deg.C/bit with default 12 bit resolution. 
      // Round by adding half denominator for positive temperatures and 
      // subtracting half denominator for negative temperatures. 

      if (scratch[8] == dowcrc) 
      { 
         float t; 
          
         temperature = (signed int16) make16(scratch[1],scratch[0]); 
          
         if (temperature >= 0) 
            t = ((float)temperature + 8.0)/16.0; 
         else 
            t = ((float)temperature - 8.0)/16.0; 
          
//         fprintf(PC, "%4LdC \n\r",temperature); 
          
         return (t); 
      } 
//      else 
//         fprintf(PC, "crc error | try=%d \n\r",try); 
   } 
    
   return DS1820_CONN_ERROR; 
} 
*/

float ds1820_read(int8 id){
  int8 i, tmp, scratch[9]; 
  int16 sensData, celsius, fract;
  unsigned char sign;
  float temperature;
  
  numRoms = id;
  
   if (!ow_reset())     // If a device is present and it's time to send string 
      {
       write_byte(0xCC); // Skip Rom command 
       write_byte(0x44); // Temperature convert command 
       output_float(DQ); 
       delay_ms(750);    // Max. conv. time is 750mS for 12 bit 
       ow_reset(); 
      
       if (Send_MatchRom()) { 
               write_byte(0xBE); // Read scratch pad command                
               for(i=0; i<2; i++) 
               { 
                  scratch[i]= read_byte(); 
               }            
               // raw sensor data (16bit) 
               sensData = make16(scratch[1], scratch[0]); 
                
               // check negative 
               if (bit_test(sensData, 15)) 
               { 
                  sign = '-'; 
                  sensData = ~sensData + 1;                
               } else 
                     sign = '+'; 
    
               fract = 0; 
               tmp = sensData&0xF;        // obtain the fractional part nibble                
               celsius = sensData >> 4 ;  // calculate the whole number part 
                
               if (tmp == 0xFF) 
                   celsius = celsius + 1;    // Calculate the fractional part            
               else 
                  for (i=0; i<tmp; i++)    
                     fract = fract + 0625;    
               
               //lcd_gotoxy(1,1);
               //printf(lcd_putc,"\fDevice_%u;%c%03lu.%04lu;", numRoms, sign, celsius, fract);  

               if (sign=='+') temperature = 0 + (celsius + 0.0001*fract);
               if (sign=='-') temperature = 0 - (celsius + 0.0001*fract);
               
               return temperature;
               //lcd_gotoxy(1,2);
               //printf(lcd_putc,"%3.1f",temperature);
      }  
  
      return DS1820_CONN_ERROR; 
   }
}

