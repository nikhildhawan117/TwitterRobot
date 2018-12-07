/*
 * File:        TFT, keypad, DAC, LED, PORT EXPANDER test
 *              With serial interface to PuTTY console
 * Authors:     Nikhil Dhawan, Ian Kranz, Sofya Calvin
 * For use with Sean Carroll's Big Board
 * http://people.ece.cornell.edu/land/courses/ece4760/PIC32/target_board.html
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "twsound.h"
#include "config_1_2_3.h"
// threading library
#include "pt_cornell_1_2_3.h"
// yup, the expander
#include "port_expander_brl4.h"

////////////////////////////////////
// graphics libraries
// SPI channel 1 connections to TFT
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
// need for sin function
#include <math.h>
////////////////////////////////////

// lock out timer 2 interrupt during spi comm to port expander
// This is necessary if you use the SPI2 channel in an ISR.
// The ISR below runs the DAC using SPI2
#define start_spi2_critical_section INTEnable(INT_T2, 0)
#define end_spi2_critical_section INTEnable(INT_T2, 1)

////////////////////////////////////

/* Demo code for interfacing TFT (ILI9340 controller) to PIC32
 * The library has been modified from a similar Adafruit library
 */
// Adafruit data:
/***************************************************
  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// string buffer
char buffer[60];

////////////////////////////////////
// DAC ISR
// A-channel, 1x, active
#define DAC_config_chan_A 0b0011000000000000
// B-channel, 1x, active
#define DAC_config_chan_B 0b1011000000000000
// DDS constant
#define two32 4294967296.0 // 2^32 
#define F_OUT 44000
#define SYS_FREQ 40000000
#define table_size 9677
#define Fs 2200

//== Timer 2 interrupt handler ===========================================
volatile unsigned int DAC_data ;// output value
volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
volatile int spiClkDiv = 4 ; // 10 MHz max speed for port expander!!
// the DDS units:
volatile unsigned int phase_accum_main, phase_incr_main=400.0*two32/Fs ;//
// DDS sine table
#define sine_table_size 256
volatile int sin_table[sine_table_size];

int exp_on[16] = {2, 4, 8, 15, 30, 58, 114, 224, 439, 863, 1696, 3335, 6556, 12888, 25337, 49813};
int blink_on[16] = {3, 3, 3, 3, 3, 3, 3, 3, 49813, 49813, 49813, 49813, 49813, 49813, 49813, 49813};

int light_phase = 0;
int light_index = 0;

// === print a line on TFT =====================================================
// print a line on the TFT
// string buffer
char buffer[60];
void printLine(int line_number, char* print_buffer, short text_color, short back_color){
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 10 ;
    // erase the pixels
    tft_fillRoundRect(0, v_pos, 239, 8, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(0, v_pos);
    tft_setTextSize(1);
    tft_writeString(print_buffer);
}

void printLine2(int line_number, char* print_buffer, short text_color, short back_color){
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 20 ;
    // erase the pixels
    tft_fillRoundRect(0, v_pos, 239, 16, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(0, v_pos);
    tft_setTextSize(2);
    tft_writeString(print_buffer);
}


void draw_joy_face(){
    //Left eye
    tft_drawLine(70,100,100,50,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(100,50,130,100,ILI9340_YELLOW);// x1,y1,x2,y2,color
    //Right eye
    tft_drawLine(190,100,220,50,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(220,50,250,100,ILI9340_YELLOW);// x1,y1,x2,y2,color
    //Mouth
    tft_drawLine(120,140,160,180,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(160,180,200,140,ILI9340_YELLOW);// x1,y1,x2,y2,color
}

void clear_joy_face(){
    //Left eye
    tft_drawLine(70,100,100,50,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(100,50,130,100,ILI9340_BLACK);// x1,y1,x2,y2,color
    //Right eye
    tft_drawLine(190,100,220,50,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(220,50,250,100,ILI9340_BLACK);// x1,y1,x2,y2,color
    //Mouth
    tft_drawLine(120,140,160,180,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(160,180,200,140,ILI9340_BLACK);// x1,y1,x2,y2,color
}

void draw_anger_face(){
    //Left eye
    tft_drawRect(90,50,40,50,ILI9340_YELLOW);// x1,y1,w,h,color
    //Right eye
    tft_drawRect(190,50,40,50,ILI9340_YELLOW);// x1,y1,w,h,color
    //Mouth
    tft_drawLine(120,180,160,140,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(160,140,200,180,ILI9340_YELLOW);// x1,y1,x2,y2,color
    //Eyebrows
    tft_drawLine(130,25,150,45,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(170,45,190,25,ILI9340_YELLOW);// x1,y1,x2,y2,color
}

void clear_anger_face(){
    //Left eye
    tft_drawRect(90,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    //Right eye
    tft_drawRect(190,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    //Mouth
    tft_drawLine(120,180,160,140,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(160,140,200,180,ILI9340_BLACK);// x1,y1,x2,y2,color
    //Eyebrows
    tft_drawLine(130,25,150,45,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(170,45,190,25,ILI9340_BLACK);// x1,y1,x2,y2,color
}

void draw_sad_face(){
    //Left eye
    tft_drawRect(90,50,40,50,ILI9340_YELLOW);// x1,y1,w,h,color
    //Right eye
    tft_drawRect(190,50,40,50,ILI9340_YELLOW);// x1,y1,w,h,color
    //Mouth
    tft_drawLine(120,180,160,140,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(160,140,200,180,ILI9340_YELLOW);// x1,y1,x2,y2,color
    //Eyebrows
    tft_drawLine(70,45,90,25,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(230,25,250,45,ILI9340_YELLOW);// x1,y1,x2,y2,color
}

void clear_sad_face(){
    //Left eye
    tft_drawRect(90,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    //Right eye
    tft_drawRect(190,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    //Mouth
    tft_drawLine(120,180,160,140,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(160,140,200,180,ILI9340_BLACK);// x1,y1,x2,y2,color
    //Eyebrows
    tft_drawLine(70,45,90,25,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(230,25,250,45,ILI9340_BLACK);// x1,y1,x2,y2,color
}

void draw_fear_face(){
    //Left eye
    tft_drawRect(90,50,40,50,ILI9340_YELLOW);// x1,y1,w,h,color
    //Right eye
    tft_drawRect(190,50,40,50,ILI9340_YELLOW);// x1,y1,w,h,color
    //Mouth
    tft_drawLine(80,170,100,150,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(100,150,120,170,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(120,170,140,150,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(140,150,160,170,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(160,170,180,150,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(180,150,200,170,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(200,170,220,150,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(220,150,240,170,ILI9340_YELLOW);// x1,y1,x2,y2,color
    //Eyebrows
    tft_drawLine(70,45,90,25,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(230,25,250,45,ILI9340_YELLOW);// x1,y1,x2,y2,color
}

void clear_fear_face(){
    //Left eye
    tft_drawRect(90,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    //Right eye
    tft_drawRect(190,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    //Mouth
    tft_drawLine(80,170,100,150,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(100,150,120,170,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(120,170,140,150,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(140,150,160,170,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(160,170,180,150,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(180,150,200,170,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(200,170,220,150,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(220,150,240,170,ILI9340_BLACK);// x1,y1,x2,y2,color
    //Eyebrows
    tft_drawLine(70,45,90,25,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(230,25,250,45,ILI9340_BLACK);// x1,y1,x2,y2,color
}

void draw_surprise_face(){
    //Left eye
    tft_drawRect(90,50,40,50,ILI9340_YELLOW);// x1,y1,w,h,color
    //Right eye
    tft_drawRect(190,50,40,50,ILI9340_YELLOW);// x1,y1,w,h,color
    //Mouth
    tft_drawCircle(160,160,10,ILI9340_YELLOW);// x1,y1,r,color
    //Eyebrows
    tft_drawLine(90,30,110,10,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(110,10,130,30,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(190,30,210,10,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(210,10,230,30,ILI9340_YELLOW);// x1,y1,x2,y2,color
}

void clear_surprise_face(){
    //Left eye
    tft_drawRect(90,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    //Right eye
    tft_drawRect(190,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    //Mouth
    tft_drawCircle(160,160,10,ILI9340_BLACK);// x1,y1,r,color
    //Eyebrows
    tft_drawLine(90,30,110,10,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(110,10,130,30,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(190,30,210,10,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(210,10,230,30,ILI9340_BLACK);// x1,y1,x2,y2,color
}

void draw_love_face(){
    //Left eye
    tft_drawLine(70,100,100,50,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(100,50,130,100,ILI9340_YELLOW);// x1,y1,x2,y2,color
    //Right eye
    tft_drawLine(190,100,220,50,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(220,50,250,100,ILI9340_YELLOW);// x1,y1,x2,y2,color
    //Mouth
    tft_drawLine(150,180,170,170,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(170,170,150,160,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(150,160,170,150,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(170,150,150,140,ILI9340_YELLOW);// x1,y1,x2,y2,color
}

void clear_love_face(){
    //Left eye
    tft_drawLine(70,100,100,50,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(100,50,130,100,ILI9340_BLACK);// x1,y1,x2,y2,color
    //Right eye
    tft_drawLine(190,100,220,50,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(220,50,250,100,ILI9340_BLACK);// x1,y1,x2,y2,color
    //Mouth
    tft_drawLine(150,180,170,170,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(170,170,150,160,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(150,160,170,150,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(170,150,150,140,ILI9340_BLACK);// x1,y1,x2,y2,color
}

void draw_funny_face(){
    //Left eye
    tft_drawLine(90,105,140,75,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(140,75,90,45,ILI9340_YELLOW);// x1,y1,x2,y2,color
    //Right eye
    tft_drawLine(230,105,180,75,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(180,75,230,45,ILI9340_YELLOW);// x1,y1,x2,y2,color
    //Mouth
    tft_drawLine(120,140,160,180,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(160,180,200,140,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(120,140,200,140,ILI9340_YELLOW);// x1,y1,x2,y2,color
}

void clear_funny_face(){
    //Left eye
    tft_drawLine(90,105,140,75,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(140,75,90,45,ILI9340_BLACK);// x1,y1,x2,y2,color
    //Right eye
    tft_drawLine(230,105,180,75,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(180,75,230,45,ILI9340_BLACK);// x1,y1,x2,y2,color
    //Mouth
    tft_drawLine(120,140,160,180,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(160,180,200,140,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(120,140,200,140,ILI9340_BLACK);// x1,y1,x2,y2,color
}

void draw_rest_face(){
    tft_drawLine(90,75,130,75,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(190,75,230,75,ILI9340_BLACK);// x1,y1,x2,y2,color
    //Left eye
    tft_drawRect(90,50,40,50,ILI9340_YELLOW);// x1,y1,w,h,color
    //Right eye
    tft_drawRect(190,50,40,50,ILI9340_YELLOW);// x1,y1,w,h,color
    //Mouth
    tft_drawLine(120,150,160,170,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(160,170,200,150,ILI9340_YELLOW);// x1,y1,x2,y2,color
    //Eyebrows
    tft_drawLine(90,30,130,30,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(190,30,230,30,ILI9340_YELLOW);// x1,y1,x2,y2,color
}
void blink_face(){
    //clear eyes
    tft_drawRect(90,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    tft_drawRect(190,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    //draw blink
    tft_drawLine(90,75,130,75,ILI9340_YELLOW);// x1,y1,x2,y2,color
    tft_drawLine(190,75,230,75,ILI9340_YELLOW);// x1,y1,x2,y2,color
}

void clear_rest_face(){
    tft_drawRect(90,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    //Right eye
    tft_drawRect(190,50,40,50,ILI9340_BLACK);// x1,y1,w,h,color
    //Mouth
    tft_drawLine(120,150,160,170,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(160,170,200,150,ILI9340_BLACK);// x1,y1,x2,y2,color
    //Eyebrows
    tft_drawLine(90,30,130,30,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(190,30,230,30,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(90,75,130,75,ILI9340_BLACK);// x1,y1,x2,y2,color
    tft_drawLine(190,75,230,75,ILI9340_BLACK);// x1,y1,x2,y2,color
}



// Predefined colors definitions (from tft_master.h)
//#define	ILI9340_BLACK   0x0000
//#define	ILI9340_BLUE    0x001F
//#define	ILI9340_RED     0xF800
//#define	ILI9340_GREEN   0x07E0
//#define ILI9340_CYAN    0x07FF
//#define ILI9340_MAGENTA 0xF81F
//#define ILI9340_YELLOW  0xFFE0
//#define ILI9340_WHITE   0xFFFF

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_color, pt_anim, pt_key, pt_serial, pt_draw, pt_face ;
// The following threads are necessary for UART control
static struct pt pt_input, pt_output, pt_DMA_output ;

// system 1 second interval tick
int sys_time_seconds ;
int updateflag = 0;
int inc = 1;
int pwm_phase = 0;
int fade_count = 3;
int turn_count = 0;
char cmd[30];

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
{   
    if (light_phase == fade_count) {
            if (inc) {
                light_index++;
            }
            else {
                light_index--;
            }
            if (light_index > 14) {
                inc = 0;
                //OpenOC4(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 2000); 
                //OpenOC5(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 2000); 
            }
            else if (light_index < 1) {
                inc = 1;
                //CloseOC4();
                //CloseOC5();
            }
            
            if (cmd[0] == 'h') {
                SetPulseOC1(3, 1);
                SetPulseOC2(3, 1);
                SetPulseOC3(3, 1);
            }
            else if (cmd[0] == 'a') {
                SetPulseOC1(blink_on[light_index], 1);
                SetPulseOC2(3, 1);
                SetPulseOC3(50000, 1);
                fade_count = 1;
            }
            else if (cmd[0] == 's') {
                SetPulseOC1(3, 1);
                SetPulseOC2(exp_on[light_index], 1);
                SetPulseOC3(50000, 1);
                fade_count = 2;
            }
            else if (cmd[0] == 'f') {
                SetPulseOC1(50000, 1);
                SetPulseOC2(blink_on[light_index], 1);
                SetPulseOC3(50000, 1);
                fade_count = 1;
            }
            else if (cmd[0] == 'e') {
                SetPulseOC1(exp_on[light_index], 1);
                SetPulseOC2(exp_on[light_index], 1);
                SetPulseOC3(50000-blink_on[light_index], 1);
                fade_count = 1;
            }
            else if (cmd[0] == 'b') {
                SetPulseOC1(exp_on[light_index], 1);
                SetPulseOC2(3, 1);
                SetPulseOC3(50000, 1);
                fade_count = 3;
            }
            else if (cmd[0] == 'u') {
                SetPulseOC1(50000, 1);
                SetPulseOC2(50000, 1);
                SetPulseOC3(3, 1);
            }
            else {
                SetPulseOC1(3, 1);
                SetPulseOC2(3, 1);
                SetPulseOC3(50000-exp_on[light_index], 1);
                fade_count = 5;
            }   
            light_phase = 0;
    }
    
    if(updateflag) {
                turn_count++;
                if (turn_count == 100) {
                    OpenOC4(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 20000); 
                    OpenOC5(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 20000);
                }
                
                else if (turn_count == 370) {
                    CloseOC4();
                    CloseOC5();
                }
     }
    else {
                turn_count = 0;
    }
    
    light_phase++;
    mT2ClearIntFlag();
}

static PT_THREAD (protothread_face(struct pt *pt))
{
    PT_BEGIN(pt);
    switch(updateflag){
        case 1:
            
            DmaChnStartTxfer(0, DMA_WAIT_NOT, 1);
            
            tft_fillRoundRect(0,0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            
            if (cmd[0] == 'h') {
                draw_joy_face();
            }
            else if (cmd[0] == 'a') {
                draw_anger_face();
            }
            else if (cmd[0] == 's') {
                draw_sad_face();
            }
            else if (cmd[0] == 'f') {
                draw_fear_face();
            }
            else if (cmd[0] == 'e') {
                draw_surprise_face();
            }
            else if (cmd[0] == 'b') {
                draw_love_face();
            }
            else if (cmd[0] == 'u') {
                draw_funny_face();
            }
            PT_YIELD_UNTIL(pt, updateflag == 0);
            break;
        default:
            tft_fillRoundRect(0,0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            draw_rest_face();
            PT_YIELD_UNTIL(pt, updateflag == 1);
            
           // PT_YIELD_TIME_msec(2000);
           // blink_face();
           // PT_YIELD_TIME_msec(500);
           // clear_rest_face();
    }
  PT_END(pt);
}

static PT_THREAD (protothread_serial(struct pt *pt))
{
    PT_BEGIN(pt);
      
      // string buffer
      
      while(1) {
            
            //spawn a thread to handle terminal input
            // the input thread waits for input
            // -- BUT does NOT block other threads
            // string is returned in "PT_term_buffer"
            // !!!! --- !!!!
            // Choose ONE of the following:
            // PT_GetSerialBuffer or PT_GetMachineBuffer
            // !!!! --- !!!!
            PT_terminate_char = '#' ;
            PT_terminate_count = 0 ;
            PT_terminate_time = 0 ;
            PT_SPAWN(pt, &pt_input, PT_GetMachineBuffer(&pt_input));
            
            if(PT_timeout==0) {
                sscanf(PT_term_buffer, "%s", cmd);
                if (cmd[0] == 'y')
                    updateflag = 0;
                else
                    updateflag = 1;
            }
            // no actual string
             else {
                 // uncomment to prove time out works
                 //mPORTAToggleBits(BIT_0);
                 // disable the command parser below
                 cmd[0] = 0 ;
             }
            //PT_YIELD_TIME_msec(500);
            /*if (cmd) {
                tft_setCursor(0, 40);
                tft_fillRoundRect(0,40, 200, 20, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                tft_writeString("Received");
            }*/
            
            // print the periods
            
            //sprintf(buffer,"RX_data=%s ", cmd);
           
                
            // never exit while
      } // END WHILE(1)
  PT_END(pt);
} // thread 3

// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; 

    
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_16, 0xC350);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // and clear the interrupt flag
    
    OpenOC1(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 3900); //
    PPSOutput(1, RPB7, OC1);
    
    OpenOC2(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 3900); //
    PPSOutput(2, RPB8, OC2);
    
    OpenOC3(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 3900); //
    PPSOutput(4, RPB9, OC3);
    
    OpenOC4(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 4000); //
    PPSOutput(3, RPA2, OC4);
    
    OpenOC5(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 4000); //
    PPSOutput(3, RPA4, OC5);
    
    CloseOC4();
    CloseOC5();
    
    //AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    
    //set timer value to sent samples at 44100Hz
    //T2_INT_OFF
    int timer_limit_1 = SYS_FREQ/(F_OUT);
    OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_1 , timer_limit_1);
    //ConfigIntTimer2(T2_INT_OFF);
    SpiChnOpen(SPI_CHANNEL2, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN 
            | SPI_OPEN_CKE_REV | SPICON_FRMEN | SPICON_FRMPOL, 2);
    
    PPSOutput(2, RPB5, SDO2);
    PPSOutput(4, RPA3, SS2);
    
    DmaChnOpen(0 ,0, DMA_OPEN_DEFAULT);
    DmaChnSetTxfer(0, AllDigits, (void*)&SPI2BUF, table_size*2, 2, 2);
    DmaChnSetEventControl(0, DMA_EV_START_IRQ(_TIMER_3_IRQ));
    DmaChnEnable(0);
  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  PT_INIT(&pt_serial);
  PT_INIT(&pt_draw);
  PT_INIT(&pt_face);

  // init the display
  // NOTE that this init assumes SPI channel 1 connections
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(1); // Use tft_setRotation(1) for 320x240

  // seed random color
  srand(1);
  
  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_serial(&pt_serial));
      PT_SCHEDULE(protothread_face(&pt_face));
      //PT_SCHEDULE(protothread_face(&pt_serial));
      }
  } // main

// === end  ======================================================
