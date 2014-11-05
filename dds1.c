// dds.c 

#include <avr/io.h>
#include <stdlib.h>
#define F_CPU 16000000UL
#include <avr/delay.h>
#include <string.h>

void cursor_home(void);
void home_line2(void);      
void fill_spaces(void);
void string2lcd(char *lcd_str);

#define DELAY 65000 // for waiting
#define delay_count 4000  //uP clock in MHz divided by 4000

/*******************************************************/
//delays by 1ms increments
void delay_ms1(uint8_t ms)
{
  uint16_t cnt;
    asm volatile (
      "\n"
      "L_dl1%=:"       "\n\t"
      "mov %A0, %A2"   "\n\t"
      "mov %B0, %B2"   "\n"
      "L_dl2%=:"       "\n\t"
      "sbiw %A0, 1"    "\n\t"
      "brne L_dl2%="   "\n\t"
      "dec %1"         "\n\t"
      "brne L_dl1%="    "\n\t"
       : "=&w" (cnt)
       : "r" (ms), "r" (delay_count)
    );
}
/*******************************************************/

char lcd_str[16];  //holds string to send to lcd  

void strobe_lcd(void){
	//twiddles bit 3, PORTF creating the enable signal for the LCD
	PORTF |=  0x08;
	PORTF &= ~0x08;
}          
 
void clear_display(void){
	SPDR = 0x00;    //command, not data
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	SPDR = 0x01;    //clear display command
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	strobe_lcd();   //strobe the LCD enable pin
	delay_ms1(2);   //obligatory waiting for slow LCD
}         

void cursor_home(void){
	SPDR = 0x00;    //command, not data
        delay_ms1(5);
	//while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	SPDR = 0x02;   // cursor go home position
        delay_ms1(5);
	//while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	strobe_lcd();
	delay_ms1(1);
}         
  
void home_line2(void){
	SPDR = 0x00;    //command, not data
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	SPDR = 0xC0;   // cursor go home on line 2
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	strobe_lcd(); 
	delay_ms1(1);  
}                           
 
void fill_spaces(void){
	int count;
	for (count=0; count<=15; count++){
	  SPDR = 0x01; //set SR for data
	  while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	  SPDR = 0x20; 
	  while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	  strobe_lcd();
	  delay_ms1(1);
	}
}  
   
void char2lcd(char a_char){
	//sends a char to the LCD
	//usage: char2lcd('H');  // send an H to the LCD
	SPDR = 0x01;   //set SR for data xfer with LSB=1
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	SPDR = a_char; //send the char to the SPI port
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	strobe_lcd();  //toggle the enable bit
	delay_ms1(1); //wait the prescribed time for the LCD to process
}
  
  
void string2lcd(char *lcd_str){

	//sends a string to LCD
	int count;
	for (count=0; count<=(strlen(lcd_str)-1); count++){
	  SPDR = 0x01; //set SR for data
	  while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	  SPDR = lcd_str[count]; 
	  while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	  strobe_lcd();
	  delay_ms1(1);
	}                  
} 

void lcd_init(void){
	int i;
	DDRF |= 0x08;  //port F bit 3 is the enable for LCD
        //initalize the LCD to receive data
	delay_ms1(15);   
	for(i=0; i<=2; i++){ //do funky initalize sequence 3 times
	  SPDR = 0x00;
	  while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	  SPDR = 0x30;
	  while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	  strobe_lcd();
	  delay_ms1(7);
	}

	SPDR = 0x00;
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	SPDR = 0x38;
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	strobe_lcd();
	delay_ms1(5);   

	SPDR = 0x00;
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	SPDR = 0x08;
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	strobe_lcd();
	delay_ms1(5);

	SPDR = 0x00;
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	SPDR = 0x01;
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	strobe_lcd();
	delay_ms1(5);   

	SPDR = 0x00;
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	SPDR = 0x06;
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	strobe_lcd();
	delay_ms1(5);

	SPDR = 0x00;
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	SPDR = 0x0E;
	while (!(SPSR & 0x80)) {}	// Wait for SPI transfer to complete
	strobe_lcd();
	delay_ms1(5);
} 

void spi_init(void){ 
 /* Run this code before attempting to write to the LCD.*/
 DDRB  |= 0x07;  //Turn on SS, MOSI, SCLK 
 //Master mode, Clock=clk/2, Cycle half phase, Low polarity, MSB first  
// SPCR=0x70;  //place SPI unit into LSB first mode
 SPCR=0x50;
 SPSR=0x01;
 }

/************************************************************************************/
int8_t encoder_chk(uint8_t encoder) {
static uint16_t state[2] = {0,0};  //holds shifted in bits from encoder
uint8_t a_pin, b_pin;

//if no encoder 1 is not specified, assume encoder 0
if (encoder == 1) { a_pin = 5; b_pin = 4;}
else              { a_pin = 3; b_pin = 2;}

state[encoder] = (state[encoder] << 1) | (! bit_is_clear(PINE, a_pin)) | 0xE000;
  if (state[encoder] == 0xF000){   //falling edge detected on A output
    if (bit_is_set(PINE, b_pin)) //get state of "B" to determine direction
      return 1; //detected CW rotation
    else
      return 0; //detected CCW rotation
  }
  else return -1; //no movement detected
} //encoder_chk

/***********************************************************************/


/****************************  update_dds  ****************************/
void update_dds(int32_t tuning){
//void  *tuning_ptr;  //void pointer 
uint8_t i;

  SPCR=0x70;  //place SPI unit into LSB first mode
  PORTE |= 0x01; //debug
  
//tuning_ptr = &tuning;//tuning_ptr holds address of variable tuning
//for(i=0;i<=3;i++){
// SPDR = *((uint8_t*)tuning_ptr + 1);
// while(bit_is_clear(SPSR,SPIF)){};
//}
  
  
  SPDR = tuning;       //byte 0
  while(bit_is_clear(SPSR,SPIF)){};
  tuning = tuning >> 8;
  SPDR = tuning;       //byte 1
  while(bit_is_clear(SPSR,SPIF)){};
  tuning = tuning >> 8;
  SPDR = tuning;       //byte 2
  while(bit_is_clear(SPSR,SPIF)){};
  tuning = tuning >> 8;
  SPDR = tuning;       //byte 3
  while(bit_is_clear(SPSR,SPIF)){};

  SPDR = 0x00;         //control byte
  while(bit_is_clear(SPSR,SPIF)){};
  PORTB |=  0x01;      //FQ_UD high  
  PORTB &= ~0x01;      //FQ_UD low

  SPCR=0x50; //put SPI unit back into MSB first mode
  PORTE &= ~0x01; //debug
}
/**********************************************************************/


/********************** init_dds **************************************/
void init_dds(){
  PORTB |=  0x10; //force reset
  PORTB &= ~0x10; //unforce reset
  //Wclk sequence
  PORTB |=  0x02; //clock high  
  PORTB &= ~0x02; //clock low
  //fq_ud sequence 
  PORTB |=  0x01; //FQ_UD high  
  PORTB &= ~0x01; //FQ_UD low
}
/**********************************************************************/

/********************** compute_tuning_wd******************************/

uint32_t compute_tuning_wd(uint32_t frequency){
uint32_t tuning_wd, tuning_wd1, tuning_wd2, tuning_wd3, tuning_wd4, tuning_wd5, tuning_wd6, tuning_wd7;
  //dependent on 125Mhz input oscillator
  //  tuning_wd = frequency * 34.3597  frequency / 0.0291038
  tuning_wd1 = frequency  * 34; 
  tuning_wd2 = (frequency * 3)/10; 
  tuning_wd3 = (frequency * 5)/100;
  tuning_wd4 = (frequency * 9)/1000;
  tuning_wd5 = (frequency * 7)/10000;
  tuning_wd6 = (frequency * 3)/100000;
  tuning_wd7 = (frequency * 8)/1000000;
  tuning_wd = tuning_wd1 + tuning_wd2 + tuning_wd3 + tuning_wd4 + tuning_wd5 + tuning_wd6 + tuning_wd7; 
  return (tuning_wd);
  }
/**********************************************************************/


/********************** print_out ******************************/
/*
void print_out(){
  clear_display();
  tuning_wd = compute_tuning_wd(frequency);
  update_dds(tuning_wd);

  cursor_home();
  ultoa(frequency, lcd_freq_str0, 10);   //convert to ascii string
  string2lcd(lcd_freq_str0);

  home_line2();
  ultoa(tuning_wd, lcd_freq_str1, 10);   //convert to ascii string
  string2lcd(lcd_freq_str1);
 }
 */
/**********************************************************************/

int main (void)
  {
int8_t  i;
uint32_t frequency;
uint32_t tuning_wd;

char lcd_freq_str0[16];  //holds string of frequency
char lcd_freq_str1[16];  //holds string of frequency

DDRB |= 0x17; //add bit for dds unit 
DDRE |= 0x01;
PORTE |= 0x3C; //turn on encoder pin pullups

init_dds(); //init the dds unit, must be done before spi or lcd init
spi_init(); //init the spi port
lcd_init(); //ini the lcd unit

//tuning = 0x15E9E1B1;  //10.7 Mhz with LSB first
//update_dds(tuning); 

frequency = 10700000; //initially 10.7Mhz
//dds tuning word will be 15E9E1AC instead of ......B1
//error is 5*.029 hz  or about 1hz

while (1){
  i = encoder_chk(0);
  switch(i){
    case(0) : frequency += 10000; break;
    case(1) : frequency -= 10000; break;
    case(-1): break;
  }//switch

 if (i != -1){ 
//   print_out();
  clear_display();
  tuning_wd = compute_tuning_wd(frequency);
  update_dds(tuning_wd);

  cursor_home();
  ultoa(frequency, lcd_freq_str0, 10);   //convert to ascii string
  string2lcd(lcd_freq_str0);

  home_line2();
  ultoa(tuning_wd, lcd_freq_str1, 10);   //convert to ascii string
 string2lcd(lcd_freq_str1);

  }//if
} //while(1)
}//main
