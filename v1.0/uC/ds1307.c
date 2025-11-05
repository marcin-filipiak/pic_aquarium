/*
ZEGAR CZASU RZECZYWISTEGO 
  DS1307
*/  

#define DS1307_SDA  PIN_B0
#define DS1307_SCL  PIN_B1

#use i2c(master, sda=DS1307_SDA, scl=DS1307_SCL)

//==========================
// initial DS1307
//==========================
void init_DS1307()
{
   output_float(DS1307_SCL);
   output_float(DS1307_SDA);
}
//==========================
// write data one byte to
// DS1307
//==========================
void write_DS1307(byte address, BYTE data)
{
   short int status;
   char x;
   i2c_start();
   i2c_write(0xd0);
   i2c_write(address);
   i2c_write(data);
   i2c_stop();
   i2c_start();
   status=i2c_write(0xd0);
   x = 0xd0;
   while(status==1 || x<5)
   {
      i2c_start();
      status=i2c_write(0xd0);
      x++;
   }
}
//==========================
// read data one byte from DS1307
//==========================
BYTE read_DS1307(byte address)
{
   BYTE data;
   i2c_start();
   i2c_write(0xd0);
   i2c_write(address);
   i2c_start();
   i2c_write(0xd1);
   data=i2c_read(0);
   i2c_stop();
   return(data);
}
