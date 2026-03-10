/****************************************************************************
 Module
   EncoderService.c

 Revision
   1.0.1

 Description
   This is a template file for implementing a simple service under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "EncoderService.h"
#include "dbprintf.h"
#include "MotorService.h"

#include <xc.h>
#include <sys/attribs.h>

/*----------------------------- Module Defines ----------------------------*/

// RPM update interval
#define UPDATE_INTERVAL 100

// Conversion factor for PBCLK 20MHz, prescaler of 8, and 2048 pulses per revolution 
#define RPM_CONVERSION 73242

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static uint16_t CurrentEncoderPeriod = 0;

static uint16_t EncoderCount = 0;

static uint8_t numOfLEDS = 0;

static uint16_t TargetRPM = 0;

//static uint16_t CurrentRPM = 0;

static float SumError = 0;

const float Kp = 0.05f;
const float Ki = 0.1f;
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitEncoderService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/
bool InitEncoderService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
  // Set Encoder A pin as digital input
  TRISBbits.TRISB13 = 1;
  ANSELBbits.ANSB13 = 0;
  
  IC1R = 0b0011;
  
  T2CONbits.ON = 0; // Timer2 off
  T2CONbits.TCS = 0; // PBClk as source
  T2CONbits.TCKPS = 0b011; // divide PBClk by 8
  TMR2 = 0; // Reset timer
  PR2 = 0xFFFF; // max period
  T2CONbits.ON = 1; // Timer2 on
  
  T4CONbits.ON = 0; // Timer4 off
  T4CONbits.TCS = 0; // PBClk as source
  T4CONbits.TCKPS = 0b011; // divide PBClk by 8
  TMR4 = 0; // Reset timer
  PR4 = 4999; // 2ms period
  IPC4bits.T4IP = 2; // priority
  IFS0CLR = _IFS0_T4IF_MASK; // clear flag
  IEC0SET = _IEC0_T4IE_MASK; // enable interrupt
  T4CONbits.ON = 1; // Timer4 on
  
  IC1CONbits.ON = 0; // IC1 off
  IC1CON = 0;
  IC1CONbits.ICTMR = 1; // Assign timer 2
  IC1CONbits.ICM = 0b011; // capture on rising edge
  
  // empty FIFO
  while (IC1CONbits.ICBNE) {
      volatile uint16_t dump = IC1BUF;
  }
    
  IFS0CLR = _IFS0_IC1IF_MASK; // clear flag
  IEC0SET = _IEC0_IC1IE_MASK; // enable interrupt
  IPC1bits.IC1IP = 5; // priority
  
  IC1CONbits.ON = 1; // IC1 on
  
  __builtin_enable_interrupts();

  // Initialize I/O pin as output and set to low
  TRISAbits.TRISA4 = 0;
  LATAbits.LATA4 = 0;
  
  // Start RPM update timer
  ES_Timer_InitTimer(RPM_UPDATE_TIMER, UPDATE_INTERVAL);
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/****************************************************************************
 Function
     PostEncoderService

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostEncoderService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunEncoderService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunEncoderService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
  switch (ThisEvent.EventType)
  {
    case ES_INIT:
      
      break;
     
    case ES_TIMEOUT: 
    {
        // Conversion of period to RPM
        uint16_t RPMVal = RPM_CONVERSION/CurrentEncoderPeriod;

        // Serial print RPM
//        DB_printf( "RPM: %u\n", RPMVal);
        
        // Reset RPM update timer
        ES_Timer_InitTimer(RPM_UPDATE_TIMER, UPDATE_INTERVAL);
        break;
    }
    case ES_UPDATE_TARGET: 
    {
      // Update target RPM based on ADC value
      TargetRPM = 80*(ThisEvent.EventParam/1023.0);
      break;
    }
    

    default:
      break;
  }
  
  return ReturnEvent;
}


void __ISR(_INPUT_CAPTURE_1_VECTOR, IPL5SOFT) IC1_ISR(void) {
    IFS0CLR = _IFS0_IC1IF_MASK; // clear flag
    static uint16_t lastTimeVal = 0;
    
    // Clear buffer
    while (IC1CONbits.ICBNE) {
        uint16_t currentTimeVal = IC1BUF;
        // Calculate period of encoder
        uint16_t dt = currentTimeVal-lastTimeVal;
        lastTimeVal = currentTimeVal;

        // Update global value
        CurrentEncoderPeriod = dt;
    }
    
}

void __ISR(_TIMER_4_VECTOR, IPL2SOFT) T4_ISR(void) {
    LATAbits.LATA4 = 1; // I/o Pulse
    IFS0CLR = _IFS0_T4IF_MASK; // clear flag
    
    // Calculate RPM
    float CurrentRPM = RPM_CONVERSION/CurrentEncoderPeriod;
    
    // RPM ceiling for bad encoder periods
    if (CurrentRPM > 150) {
        CurrentRPM = 150;
    }
    
    // Calculate RPM Error
    float RPMError = TargetRPM - CurrentRPM;
    SumError += RPMError;
    
    // Calculate the requested duty
    float RequestedDuty = Kp*(RPMError + (Ki * SumError));
    
    // Anti Windup
    if (RequestedDuty > 100){
        RequestedDuty = 100.0;
        SumError -= RPMError;
    } else if (RequestedDuty < 0) {
        RequestedDuty = 0.0;
        SumError -= RPMError;
    }
    
    // Post update to motor service
//    UpdateRPM(RequestedDuty,1);
            
    LATAbits.LATA4 = 0; // I/O pulse
}

/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

