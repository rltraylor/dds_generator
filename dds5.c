// dds5.c
//Uses the unified lcd code on the mega8 board
//Has state encoded enocder machine

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include <string.h>
#include <avr/interrupt.h>

#include "unified_lcd_8bit.h"
#include "delay.h"

uint32_t frequency;
uint32_t tuning_wd;
char     lcd_string_array[32];
int32_t  freq_inc_array[6] = {10,100,1000,10000,100000,1000000};
volatile int32_t increment_amount = 10; //gets changed in interrupt routine

volatile int8_t  cnt = 0; 
volatile uint8_t present_state;


/******************************************************************************/
//                                spi_init
//
void spi_init(void){ 
// Run this mega8 specific code before attempting to use SPI.
 DDRB  |= 0x2C;                 //Turn on SS, MOSI, SCLK 
 SPCR  |= (1<<SPE) | (1<<MSTR); //MSTR mode, Half phase, Low polarity, MSB first  
 SPSR  |= (1<<SPI2X);           //SPI Clock = 8Mhz
 //DEBUG
 }
/******************************************************************************/

/******************************************************************************/
//                             button0_chk                                   
//
//Checks for pushbutton closure. Pushbutton is on PORTD bit zero.
/******************************************************************************/

uint8_t button0_chk() {
  static uint16_t state = 0; //holds present state
  state = (state << 1) | (! bit_is_clear(PIND, 0)) | 0xE000;
  if (state == 0xF000) return 1;
  return 0;
}//button0_chk
/******************************************************************************/

/******************************************************************************/
//                        encoder_chk
//Checks state of the encoder. Returns 1 if CW, 0 if CCW, -1 if no change.
//Specific to PORTC pins 0,1.

int8_t encoder_chk(){
static uint16_t state = 0;  //holds shifted in bits from encoder
uint8_t a_pin, b_pin;

a_pin = 1; b_pin = 0; //encoder is on port C bits 0 and 1

state = (state << 1) | (! bit_is_clear(PINC, a_pin)) | 0xE000;
  if (state == 0xF000){             //falling edge detected on A output
    if (bit_is_set(PINC, b_pin))    //get state of "B" to determine direction
      return 1;   //detected CW rotation
    else
      return 0;   //detected CCW rotation
  }
  else return -1; //no movement detected
} //encoder_chk

/******************************************************************************/

/******************************************************************************/
//                        encoder_chk1
//Checks state of the encoder. Returns 1 if CW, 0 if CCW, -1 if no change.
//Specific to PORTC pins 0,1.  Simply won't work if the encoder cannot 
//guarntee that while one output is bouncing that the other is not.

int8_t encoder_chk1(){

uint8_t new_state;
int8_t  ret_val=-1; //initalized to "no rotation" value    
  
new_state = PINC & 0x03;  //get value of encoder pins 

  switch(present_state){
    case 0x03 : if      (new_state == 0x02) {cnt--;} 
                else if (new_state == 0x01) {cnt++;}             
                break; 
    case 0x02 : if      (new_state == 0x03) {if (cnt == 3){cnt=0; ret_val=1;} //completed turn
                                             else cnt++;}   //bouncing
                else if (new_state == 0x00) {cnt--;}
                break;
    case 0x01 : if      (new_state == 0x03) {if (cnt ==-3){cnt=0; ret_val=0;} //completed turn
                                             else cnt--;}   //bouncing
                else if (new_state == 0x00) {cnt++;}
                break;
    case 0x00 : if      (new_state == 0x02) {cnt++;} 
                else if (new_state == 0x01) {cnt--;}             
                break;
  }
 present_state=new_state;  //assign next state
 return(ret_val);

} //encoder_chk1

/******************************************************************************/

/****************************  update_dds  ***********************************/
void update_dds(int32_t tuning){
//void  *tuning_ptr;  //void pointer 
//uint8_t i;

  SPCR=0x70;  //place SPI unit into LSB first mode
  
//tuning_ptr = &tuning;//tuning_ptr holds address of variable tuning
//for(i=0;i<=3;i++){
// SPDR = *((uint8_t*)tuning_ptr + 1);
// while(bit_is_clear(SPSR,SPIF)){};
//}
  
  
//  PORTC |= 0x20;  PORTC &= ~0x20;
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
  PORTB |=  0x04;      //FQ_UD high  
  PORTB &= ~0x04;      //FQ_UD low

  SPCR=0x50; //put SPI unit back into MSB first mode
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
/******************************************************************************/

/****************************** long2ascii ************************************/
//Converts uint32_t to ascii without terminator and without leading zeros
//Field size is the size of the field to be displayed. It places the characters
//in the array passed to it.
//
void long2ascii(char *str_ptr, uint32_t long_val, uint8_t field_size){
  str_ptr += (field_size-1);  
  while(field_size--){
    if(long_val==0) {*str_ptr-- = ' ';} //if zero, print space, not leading zeros
    else            {*str_ptr-- = (long_val%10) + '0';} //else print character
   long_val /= 10;
  }//while
} 
/******************************************************************************/

/******************************************************************************/

/******************************************************************************/
//                          timer/counter 2 ISR                        
//When the TCNT2 compare interrupt occurs, the pushbutton is checked and the lcd
//is updated every mod 16 times. Interrupts occurat 256uS intervals.                     

ISR(TIMER2_COMP_vect){
  static uint8_t counter=0;  //count interrupts to time lcd refresh
  static uint8_t i=0;        // count increment amounts 

  counter++;
  //refresh lcd every sixteen interrupts (8mS)

  if((counter % 32)==0){refresh_lcd(lcd_string_array);}


//check button zero to see if increment value has changed
  if(button0_chk()){
    i++;
    i = (i % 6); //six elements in array
    increment_amount = freq_inc_array[i];
  }


//  PORTC |= 0x20;  delay_us(10); PORTC &= ~0x20;
//check encoder to see if the frequency has changed       
  
  switch(encoder_chk()){
    case(0) : frequency += increment_amount; 
              tuning_wd = compute_tuning_wd(frequency);
              update_dds(tuning_wd);
              break; //CW rotation
    case(1) : frequency -= increment_amount; 
              tuning_wd = compute_tuning_wd(frequency);
              update_dds(tuning_wd);
              break; //CCW rotation
    case(-1): break;                                             //no rotation
  }//switch


}//ISR
/******************************************************************************/

/****************************** init_dds **************************************/
//Initalizes the AD9851 DDS unit. Run spi_init before init_dds.
//
//PORTB bit   Function  In/Out
//   PB0       RESET      out
//   PB2       FQ_UD      out
//   PB5       WCLK       out
//   PB3       SDATA      out

void init_dds(){
  PORTB |=  0x01; //force reset
  PORTB &= ~0x01; //unforce reset
  //wclk sequence
  PORTB |=  0x20; //clock high  
  PORTB &= ~0x20; //clock low
  //fq_ud sequence 
  PORTB |=  0x04; //FQ_UD high  
  PORTB &= ~0x04; //FQ_UD low
}
/**********************************************************************/




int main (void){
int8_t  i;
//                          PORTB:  0 0 0 0 1 0 1 1
//                  (unused) bit 7--| | | | | | | |--bit 0 (DDS reset)
//                  (unused) bit 6----| | | | | |----bit 1 (unused   )
//                (DDS wclk) bit 5------| | | |------bit 2 (DDS fq_ud)
//        (MISO programming) bit 4--------| |--------bit 3 (DDS sdata)    

//                          PORTC:  0 0 0 0 1 0 1 1
//                  (unused) bit 7--| | | | | | | |--bit 0 (encoder A)
//                  (unused) bit 6----| | | | | |----bit 1 (encoder B)
//                  (unused) bit 5------| | | |------bit 2 (unused)   
//                  (unused) bit 4--------| |--------bit 3 (unused)       
//DEBUG PORT ENABLE
DDRC |= 0x20; //port C bit 5 is for debug

DDRB  |=  0x01; //add reset line for dds unit 

DDRD  &= ~(0x01); //PORTD bit zero is pushbutton
PORTD |=   0x01;  //pullup on for pushbutton  

DDRC  &= ~(0x03); //PORTC bits 0 and 1 are inputs for encoder
PORTC |=   0x03; //Turn on pullups for encoder pins 

init_dds(); //init the dds unit, must be done before spi or lcd init
spi_init(); //init the spi port
lcd_init(); //init the lcd unit

//Mega8 TCNT2 setup for CTC mode 
TCCR2 |=  (1<<CS22) | (1<<WGM21); //prescale by (now by 8) 64, counter clock is 8000ns/tick
OCR2  =  15;                      //interrupts every 131us 
TIMSK |= (1<<OCIE2);              //interrups for timer two are on

present_state = PINC & 0x03;  //get initial value of encoder pins
//switch(present_state){
//  case 3 : cnt = 0; break;
//  case 2 : cnt = 3; break;
//  case 1 : cnt = 1; break;
//  case 0 : cnt = 2; break;
//}

sei();        //enable interrupts before entering loop

frequency = 10700000; //initially 10.7Mhz

while (1){
  long2ascii(lcd_string_array, frequency, 8);
  long2ascii((lcd_string_array+16), increment_amount, 8);
  _delay_ms(100);
  for(i=0;i<=31;i++){lcd_string_array[i] = (' ');} //clear display buffer 
} //while(1)
}//main
