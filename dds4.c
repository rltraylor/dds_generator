// dds4.c
//uses the unified lcd code  
//for the mega128 board

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include <string.h>
#include <avr/interrupt.h>

#include "unified_lcd_8bit.h"

uint32_t frequency;
uint32_t tuning_wd;

char lcd_freq_str0[16];  //holds string of frequency
char lcd_freq_str1[16];  //holds string of frequency

char lcd_string_array[32];


int32_t freq_inc_array[6] = {10,100,1000,10000,100000,1000000};
volatile int32_t increment_amount; //gets changed in interrupt routine

void spi_init(void){ 
// Run this code before attempting to use theSPI port.
 DDRB  |= 0x07;  //Turn on SS, MOSI, SCLK 
 SPCR  |= (1<<SPE) | (1<<MSTR);  //MSTR mode, Half phase, Low polarity, MSB first  
 SPSR  |= (1<<SPI2X); //SPI Clock = 8Mhz
 }

/******************************************************************************/
//                             button0_chk                                   
//
//Checks for pushbutton S1 closure. 
/******************************************************************************/

uint8_t button0_chk() {
  static uint16_t state = 0; //holds present state
  state = (state << 1) | (! bit_is_clear(PIND, 0)) | 0xE000;
  if (state == 0xF000) return 1;
  return 0;
}//button0_chk
/***********************************************************************/


//**********************************************************************
//                          timer/counter 1 ISR                        
//When the TCNT1 compare1A interrupt occurs, port C bit 0 is toggled 
//if the enable for it is true.  This creates a 1khz alarm sound for the 
//alarm clock.  Thus, interrupts occur at 2000hz rate or 500uS.                     
//***********************************************************************
ISR(TIMER1_COMPA_vect){
    static uint8_t counter=0;  //count interrupts to time lcd refresh
    static uint8_t i=0;        // count increment amounts 
    counter++; 
    //refresh lcd every sixteen interrupts (8mS)
    if((counter % 16)==0){refresh_lcd(lcd_string_array);} 

//check button zero to see if increment value has changed
 if(button0_chk()){
  i++;
  i = (i % 6); //six elements in array
  increment_amount = freq_inc_array[i];
  }

}//ISR
//                           DDRE:  0 0 0 0 1 0 1 1
//                  (unused) bit 7--| | | | | | | |--bit 0 (unused)
//                  (unused) bit 6----| | | | | |----bit 1 (unused)
//                  (unused) bit 5------| | | |------bit 2 (encoder A)
//                  (unused) bit 4--------| |--------bit 3 (encoder B)    

/************************************************************************************/
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
//uint8_t i;

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
//Run spi_init before init_dds
//
//PORTB bit functions
//PORTB bit   Function  In/Out
//    0        FQ_UD      out
//    1        SCK        out
//    2        MOSI       out
//    3        MISO        in
//    4        RESET      out

void init_dds(){
  DDRB  |= 0x10; //add bit for dds unit 
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

void print_out(){
//  clear_display();
  tuning_wd = compute_tuning_wd(frequency);
  update_dds(tuning_wd);

//  cursor_home();
//  ultoa(frequency, lcd_freq_str0, 10);   //convert to ascii string
  ultoa(frequency, lcd_string_array, 10);   //convert to ascii string
//  string2lcd(lcd_freq_str0);

//  home_line2();
//  ultoa(tuning_wd, lcd_freq_str1, 10);   //convert to ascii string
////  ultoa(tuning_wd, (lcd_string_array+16), 10);   //convert to ascii string
  ultoa(increment_amount, (lcd_string_array+16), 10);   //convert to ascii string
//  string2lcd(lcd_freq_str1);
 }

/**********************************************************************/

int main (void)
  {
int8_t  i;
//uint32_t frequency;
//uint32_t tuning_wd;

//char lcd_freq_str0[16];  //holds string of frequency
//char lcd_freq_str1[16];  //holds string of frequency

DDRF |= 0x08; //port f bit 3 is the lcd strobe

DDRD   = 0x00; //PORTD is all inputs
PORTD |= 0x01; //pullup on for pushbutton S1 

DDRE  |= 0x01;
PORTE |= 0x0C; //turn on encoder pin pullups
//for the dds project, only PORTE bits 2,3 are used for the encoder

init_dds(); //init the dds unit, must be done before spi or lcd init
spi_init(); //init the spi port
lcd_init(); //init the lcd unit

//TCNT1 setup  
//runs the alarm tone and times LCD refresh update
TCCR1A = 0x00; //CTC mode, normal port operation (pin disconnected) 
TCCR1B = 0x09; //sets timer1 OCR1A on every clock cycle (no prescalar)
OCR1A =  8000; //1000hz tone, interrupts at 500uS intervals 
TIMSK  |= (1<<OCIE1A); //turn on timer one interrupts for alarm sound

sei();        //enable interrupts before entering loop

frequency = 10700000; //initially 10.7Mhz

//clear the display buffer memory
for(i=0;i<=31;i++){lcd_string_array[i] = (' ');}
print_out();  //initial frequency to be displayed 

while (1){
  i = encoder_chk(0);
  switch(i){
    case(0) : frequency += increment_amount; print_out(); break;
    case(1) : frequency -= increment_amount; print_out(); break;
    case(-1): break;
  }//switch

} //while(1)
}//main
