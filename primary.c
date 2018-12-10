
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
#include "config_1_2_3.h"
// threading library
#include "pt_cornell_1_2_3.h"

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

////////////////////////////////////
// string buffer
char buffer[60];

////////////////////////////////////
// DDS constant
#define two32 4294967296.0 // 2^32 
#define Fs 100000

static struct pt pt_serial, pt_exec ;
// The following threads are necessary for UART control
static struct pt pt_input, pt_output, pt_DMA_output ;

// system 1 second interval tick
int sys_time_seconds ;
volatile int react_state = 2;
int receive_state = 0;
char handle[15]; //stores incoming tweet twitter handle
char tweet_blocks[5][64]; //5x64 array of tweet chunks
char tweet_full[320]; //stores entire tweet
char emotion[1]; //tweet emotion


//== Timer 23 interrupt handler ===========================================
void __ISR(_TIMER_23_VECTOR, ipl2) Timer23Handler(void)
{
    mT23ClearIntFlag();
    react_state = 2;
}

void print_tweet() {
    static char handle_line[25];
    static char line[24];
    tft_setCursor(10, 10);
    sprintf(handle_line,"@%s says:", handle);
    tft_writeString(handle_line);
    int i;
    int j;
    
    //load tweet full
    for (i = 0; i <5; i++) {
        for (j = 0; j < 64 && (tweet_blocks[i][0] != '-' || tweet_blocks[i][1] != '-'); j++) {
            tweet_full[64*i+j] = tweet_blocks[i][j];
        }
    }
    //print tweet
    for (i = 0; i < 180; i++) {
        line[i%24] = tweet_full[i];
        if (30+i/24*15 > 180) {
            break;
        }
        if (i%24 == 23 || i == 319) {
            tft_setCursor(10, 30+i/24*15);
            tft_writeString(line);
            memset(line, 0, 24);
        }
    }
    
    //clear tweet chunks and tweet full
    for (i = 0; i < 5; i++) {
        memset(tweet_blocks[i], 0, 64);
    }
    memset(tweet_full, 0, 320);
       
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

static PT_THREAD(protothread_execute(struct pt *pt))
{
    PT_BEGIN(pt);
    while (1) {
        
        PT_terminate_char = '\r' ;
        PT_terminate_count = 0 ;
        PT_terminate_time = 0 ;
        switch (react_state) {
            case 0: //start react
                WriteTimer23(0x00000000); //start timer
                print_tweet();
                react_state = 1; //transition to IP
                break;
            case 1: //react IP
                break;
            case 2: //stop react
                tft_fillRoundRect(0,0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
               // tft_setCursor(10, 30+2*15);
               // tft_writeString("Have you ever heard the");
                sprintf(PT_send_buffer,"%s#", "y");
                PT_SPAWN(pt, &pt_output, PutSerialBuffer(&pt_output));
                PT_YIELD_UNTIL(pt, react_state != 2);
                break;
        }
        
    }
    PT_END(pt);
}

//=== Serial terminal thread =================================================

static PT_THREAD (protothread_serial(struct pt *pt))
{
    PT_BEGIN(pt);
      // string buffer
      static char buffer[128];
      tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(2);
      clr_right;
      memset(PT_send_buffer, 0, max_chars);
      while(1) {
            int i = 0;
          
            PT_terminate_char = '\r' ;
            PT_terminate_count = 0 ;
            PT_terminate_time = 0 ;
          
            //Bluetooth code
            PT_SPAWN(pt, &pt_input, PT_GetMachineBuffer(&pt_input) );
            
            if(PT_timeout==0) {
                switch (receive_state) {
                    
                    case 0:
                        sscanf(PT_term_buffer, "%s", handle);
                        receive_state=1;
                        break;
                    case 1:
                        sscanf(PT_term_buffer, "%s", tweet_blocks[0]);
                        receive_state=2;
                        for (i = 0; i < 64; i++) {
                            if (tweet_blocks[0][i] == '-') {
                                tweet_blocks[0][i] = ' ';
                            }
                        }
                        break;
                    case 2:
                        sscanf(PT_term_buffer, "%s", tweet_blocks[1]);
                        receive_state=3;
                        for (i = 0; i < 64; i++) {
                            if (tweet_blocks[1][i] == '-') {
                                tweet_blocks[1][i] = ' ';
                            }
                        }
                        break;
                    case 3:
                        sscanf(PT_term_buffer, "%s", tweet_blocks[2]);
                        receive_state=4;
                        for (i = 0; i < 64; i++) {
                            if (tweet_blocks[2][i] == '-') {
                                tweet_blocks[2][i] = ' ';
                            }
                        }
                        break;
                    case 4:
                        sscanf(PT_term_buffer, "%s", tweet_blocks[3]);
                        receive_state=5;
                        for (i = 0; i < 64; i++) {
                            if (tweet_blocks[3][i] == '-') {
                                tweet_blocks[3][i] = ' ';
                            }
                        }
                        break;
                    case 5:
                        sscanf(PT_term_buffer, "%s", tweet_blocks[4]);
                        for (i = 0; i < 64; i++) {
                            if (tweet_blocks[4][i] == '-') {
                                tweet_blocks[4][i] = ' ';
                            }
                        }
                        receive_state=6;
                        break;
                    case 6:
                        sscanf(PT_term_buffer, "%s", emotion);
                        sprintf(PT_send_buffer,"%s#", emotion);
                        PT_SPAWN(pt, &pt_output, PutSerialBuffer(&pt_output));
                        react_state = 0;
                        receive_state=0;
                        PT_YIELD_UNTIL(pt, react_state == 2);
                        break;
                }
             }
            // never exit while
      } // END WHILE(1)
  PT_END(pt);
} // thread 3

// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; 

  // set up DAC on big board
  // timer interrupt //////////////////////////
  // Set up timer2 on,  interrupts, internal clock, prescalar 1, toggle rate
  // at 30 MHz PB clock 60 counts is two microsec
  // 400 is 100 ksamples/sec
  // 2000 is 20 ksamp/sec
  OpenTimer23(T23_ON | T23_SOURCE_INT | T23_PS_1_256, 0x002FAF08);
  // set up the timer interrupt with a priority of 2
  ConfigIntTimer23(T23_INT_ON | T23_INT_PRIOR_2);
  mT23ClearIntFlag(); // and clear the interrupt flag
  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  PT_INIT(&pt_serial);
  PT_INIT(&pt_exec);

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
      PT_SCHEDULE(protothread_execute(&pt_exec));
      }
  } // main

// === end  ======================================================
