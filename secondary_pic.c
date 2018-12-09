/*
 * File:        UART, PWM, DMA SPI, TFT
 * Authors:     Nikhil Dhawan, Ian Kranz, Sofya Calvin
 * For use with ECE 4760 Small Board
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

/* Program to receive emotion as char from primary PIC32
 * via UART and react with PWM, DMA SPI DDS, and TFT
 */

// string buffer
char buffer[60];

////////////////////////////////////
// DDS constants
#define F_OUT 44000
#define SYS_FREQ 40000000
#define table_size 9677

//exponential fade samples
int exp_on[16] = {2, 4, 8, 15, 30, 58, 114, 224, 439, 863, 1696, 3335, 6556, 12888, 25337, 49813};

//Periodic blink samples
int blink_on[16] = {3, 3, 3, 3, 3, 3, 3, 3, 49813, 49813, 49813, 49813, 49813, 49813, 49813, 49813};

int light_phase = 0; 
int light_index = 0; 

/*
 * Face drawing functions by emotion
*/
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

void draw_sarcastic_face(){
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

void clear_sarcastic_face(){
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

void draw_bored_face(){
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

void clear_bored_face(){
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

void draw_excited_face(){
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

void clear_excited_face(){
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

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_color, pt_anim, pt_key, pt_serial, pt_draw, pt_face ;
// The following threads are necessary for UART control
static struct pt pt_input, pt_output, pt_DMA_output ;

int updateflag = 0;
int inc = 1;
int pwm_phase = 0;
int fade_count = 3;
int turn_count = 0;
char cmd[30];

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
{   
	//update lights on time scale determined by fade count
    if (light_phase == fade_count) {

    		//move in current direction of light tables
            if (inc) {
                light_index++;
            }
            else {
                light_index--;
            }

            //change direction in light tables
            if (light_index > 14) {
                inc = 0;
            }
            else if (light_index < 1) {
                inc = 1;
            }
            
            //flash lights as reaction to tweet emotion
            if (cmd[0] == 'h') { //green lights on
                SetPulseOC1(3, 1);
                SetPulseOC2(3, 1);
                SetPulseOC3(3, 1);
            }
            else if (cmd[0] == 'a') { //blinking red
                SetPulseOC1(blink_on[light_index], 1);
                SetPulseOC2(3, 1);
                SetPulseOC3(50000, 1);
                fade_count = 1;
            }
            else if (cmd[0] == 's') { //fading yellow
                SetPulseOC1(3, 1); 
                SetPulseOC2(exp_on[light_index], 1);
                SetPulseOC3(50000, 1);
                fade_count = 2;
            }
            else if (cmd[0] == 'f') { //red on, yellow blink
                SetPulseOC1(50000, 1);
                SetPulseOC2(blink_on[light_index], 1);
                SetPulseOC3(50000, 1);
                fade_count = 1;
            }
            else if (cmd[0] == 'e') { //fade all lights
                SetPulseOC1(exp_on[light_index], 1);
                SetPulseOC2(exp_on[light_index], 1);
                SetPulseOC3(50000-blink_on[light_index], 1);
                fade_count = 1;
            }
            else if (cmd[0] == 'b') { //fade red
                SetPulseOC1(exp_on[light_index], 1);
                SetPulseOC2(3, 1);
                SetPulseOC3(50000, 1);
                fade_count = 3;
            }
            else if (cmd[0] == 'u') { //all on
                SetPulseOC1(50000, 1);
                SetPulseOC2(50000, 1);
                SetPulseOC3(3, 1);
            }
            else { //fade green, red dim
                SetPulseOC1(3, 1);
                SetPulseOC2(3, 1);
                SetPulseOC3(50000-exp_on[light_index], 1);
                fade_count = 5;
            }   
            light_phase = 0;
    }
    
    //modify PWM signals while in react mode
    if(updateflag) {
                turn_count++;
                //start turning at 2 seconds
                if (turn_count == 100) {
                    OpenOC4(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 20000); 
                    OpenOC5(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 20000);
                }
                
                //stop turning after ~5.5 seconds for full 360
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

//play sound and update TFT display
static PT_THREAD (protothread_face(struct pt *pt))
{
    PT_BEGIN(pt);
    switch(updateflag){
        case 1:
            
            //play notification sound
            DmaChnStartTxfer(0, DMA_WAIT_NOT, 1);
            
            //clear previous face
            tft_fillRoundRect(0,0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            
            //draw reaction face
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
            	draw_excited_face();
            }
            else if (cmd[0] == 'b') {
                draw_bored_face();
            }
            else if (cmd[0] == 'u') {
                draw_sarcastic_face();
            }
            PT_YIELD_UNTIL(pt, updateflag == 0);
            break;

        default:
            tft_fillRoundRect(0,0, 320, 240, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            draw_rest_face();
            PT_YIELD_UNTIL(pt, updateflag == 1);
    }

  PT_END(pt);
}

//recieve emotion via UART
static PT_THREAD (protothread_serial(struct pt *pt))
{
    PT_BEGIN(pt);
      
      while(1) {

      		//get serial data
            PT_terminate_char = '#' ;
            PT_terminate_count = 0 ;
            PT_terminate_time = 0 ;
            PT_SPAWN(pt, &pt_input, PT_GetMachineBuffer(&pt_input));
            
            //process string
            if(PT_timeout==0) {
                sscanf(PT_term_buffer, "%s", cmd);
                if (cmd[0] == 'y') //yield character
                    updateflag = 0;
                else //any other character
                    updateflag = 1;
            }
            // no actual string
             else {
                 cmd[0] = 0 ;
             }
      } // END WHILE(1)
  PT_END(pt);
} // thread 3

// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; 

    //Open timer for PWM for 50Hz
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_16, 0xC350);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag();

    //PWM for lights
    OpenOC1(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 3900); 
    PPSOutput(1, RPB7, OC1); //Red
    
    OpenOC2(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 3900); 
    PPSOutput(2, RPB8, OC2); //Yellow
    
    OpenOC3(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 3900); 
    PPSOutput(4, RPB9, OC3); //Green
    
    //PWM for Servos
    OpenOC4(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 4000); 
    PPSOutput(3, RPA2, OC4);
    
    OpenOC5(OC_ON | OC_TIMER2_SRC | OC_CONTINUE_PULSE , 1, 4000); 
    PPSOutput(3, RPA4, OC5);
    
    CloseOC4();
    CloseOC5();
    
    ///////////////////////////////////////////////////////////////
    
    //set timer value to sent samples at 44100Hz
    int timer_limit_1 = SYS_FREQ/(F_OUT);
    OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_1 , timer_limit_1);

    //set up SPI in framed mode
    SpiChnOpen(SPI_CHANNEL2, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN 
            | SPI_OPEN_CKE_REV | SPICON_FRMEN | SPICON_FRMPOL, 2);
    
    PPSOutput(2, RPB5, SDO2);
    PPSOutput(4, RPA3, SS2);
    
    //set up DMA channel to send twitter notification sound over SPI
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
  
  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_serial(&pt_serial));
      PT_SCHEDULE(protothread_face(&pt_face));
      }
  } // main

// === end  ======================================================
