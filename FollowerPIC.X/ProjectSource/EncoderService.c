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
#include "MotorSM.h"

#include <xc.h>
#include <sys/attribs.h>

/*----------------------------- Module Defines ----------------------------*/

#define TICKS_PER_REV      150.0
#define WHEEL_DIAMETER     80      // millimeters
#define WHEEL_BASE         0.27       // meters between wheels
#define PI                 3.14159

#define WHEEL_CIRCUMFERENCE   (PI * WHEEL_DIAMETER)
#define ARC_LENGTH_90         (PI * WHEEL_BASE / 4.0)
#define TARGET_TICKS_90       ((ARC_LENGTH_90 / WHEEL_CIRCUMFERENCE) * TICKS_PER_REV)

#define RPM_CONVERSION 1000000UL // 60 sec * 2.5MHz timer / 150 counts per revolution



/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static volatile int32_t LeftTicks = 0;
static volatile int32_t RightTicks = 0;

//static const float P_Constant = 0.1;
//static const float I_Constant = 0.03;

static volatile uint32_t LeftEncoderPeriod = 0;
static volatile uint32_t LastLeftEncoderPeriod = 0;
static volatile uint32_t LastLeftEncoderValue = 0;
static volatile uint16_t LeftNoChangeCount = 0;
static volatile int16_t LeftRPM = 0;
//static volatile int16_t TargetLeftRPM = 0;
//static volatile int16_t CurrentLeftError = 0;
//static volatile int16_t TotalLeftError = 0;
//static volatile int8_t LeftDuty = 0;

static volatile uint32_t RightEncoderPeriod = 0;
static volatile uint32_t LastRightEncoderPeriod = 0;
static volatile uint32_t LastRightEncoderValue = 0;
static volatile int16_t RightRPM = 0;
static volatile uint16_t RightNoChangeCount = 0;

//static volatile int16_t TargetRightRPM = 0;
//static volatile int16_t CurrentRightError = 0;
//static volatile int16_t TotalRightError = 0;
//static volatile int8_t RightDuty = 0;

static volatile uint16_t RollOverCount = 0;

static volatile uint16_t EncoderCount = 0;
static volatile uint16_t EncoderCountThreshold = 0;
static volatile bool CountEncoderTicks = false;

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
  // Set Encoder pins as digital input
  TRISBbits.TRISB9 = 1; // left
  TRISAbits.TRISA2 = 1; // right

  
  IC1R = 0b0000;
  IC2R = 0b0100;
  
  T2CONbits.ON = 0; // Timer2 off
  T2CONbits.TCS = 0; // PBClk as source
  T2CONbits.TCKPS = 0b011; // divide PBClk by 8
  TMR2 = 0; // Reset timer
  PR2 = 0xFFFF; // max period
  IPC2bits.T2IP = 4; // priority
  IFS0CLR = _IFS0_T2IF_MASK; // clear flag
  IEC0SET = _IEC0_T2IE_MASK; // enable interrupt
  T2CONbits.ON = 1; // Timer2 on
  
  T4CONbits.ON = 0; // Timer4 off
  T4CONbits.TCS = 0; // PBClk as source
  T4CONbits.TCKPS = 0b011; // divide PBClk by 8
  TMR4 = 0; // Reset timer
  PR4 = 9999; // 2ms period
  IPC4bits.T4IP = 2; // priority
  IFS0CLR = _IFS0_T4IF_MASK; // clear flag
  IEC0SET = _IEC0_T4IE_MASK; // enable interrupt
  T4CONbits.ON = 1; // Timer4 on
  
  IC1CONbits.ON = 0; // IC1 off
  IC1CON = 0;
  IC1CONbits.ICTMR = 1; // Assign timer 2
  IC1CONbits.ICM = 0b011; // capture on rising edge
  
  IC2CONbits.ON = 0; // IC2 off
  IC2CON = 0;
  IC2CONbits.ICTMR = 1; // Assign timer 2
  IC2CONbits.ICM = 0b011; // capture on rising edge
  
  // empty FIFO for IC1 and IC2
  while (IC1CONbits.ICBNE) {
      volatile uint16_t dump = IC1BUF;
  }
  
  while (IC2CONbits.ICBNE) {
      volatile uint16_t dump = IC2BUF;
  }
    
  IFS0CLR = _IFS0_IC1IF_MASK; // clear flag
  IEC0SET = _IEC0_IC1IE_MASK; // enable interrupt
  IPC1bits.IC1IP = 6; // priority
  
  IFS0CLR = _IFS0_IC2IF_MASK; // clear flag
  IEC0SET = _IEC0_IC2IE_MASK; // enable interrupt
  IPC2bits.IC2IP = 6; // priority
  
  IC1CONbits.ON = 1; // IC1 on
  IC2CONbits.ON = 1; // IC1 on
  
  __builtin_enable_interrupts();

  // Initialize I/O pin as output and set to low
  TRISAbits.TRISA4 = 0;
  LATAbits.LATA4 = 0;
  
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
      
    case ES_START_TURN_90:
    {
        /*
        LeftTicks = 0;
        RightTicks = 0;
        TargetTicks = TARGET_TICKS_90;
        T4CONbits.ON = 1; // enables encoder
        break;
         */
    }
     

    default:
      break;
  }
  
  return ReturnEvent;
}
/***************************************************************************
 public functions
 ***************************************************************************/
void DriveDistance (int16_t Distance){
    EncoderCount = 0;
    EncoderCountThreshold = ((float)Distance*150)/ (PI*WHEEL_DIAMETER);
    CountEncoderTicks = true;
}
// Rollover

void __ISR(_TIMER_2_VECTOR, IPL4SOFT) T2_ISR(void) {
//    __builtin_disable_interrupts();
    RollOverCount++;
    IFS0CLR = _IFS0_T2IF_MASK;
    
//    __builtin_enable_interrupts();
}


void __ISR(_INPUT_CAPTURE_1_VECTOR, IPL6SOFT) IC1_ISR(void) { // left

    // Get edge value
    uint16_t CurrentEdgeValue = IC1BUF;
    
    if (CountEncoderTicks) {
        EncoderCount++;
        if (EncoderCount >= EncoderCountThreshold) {
            ES_Event_t StopEvent;
            StopEvent.EventType = ES_STOP;
            PostMotorSM(StopEvent); 
            CountEncoderTicks = false;                    
        }
    }
    // Clear buffer
    while (IC1CONbits.ICBNE) {
        CurrentEdgeValue = IC1BUF;
    }
    // Clear mask
    IFS0CLR = _IFS0_IC1IF_MASK;
    
    if (IFS0bits.T2IF == 1 && (CurrentEdgeValue < 0x8000)) {
        RollOverCount++;
        IFS0CLR = _IFS0_T2IF_MASK;
    }
    uint32_t EdgeTotal = ((uint32_t)RollOverCount << 16 | CurrentEdgeValue);
    LeftEncoderPeriod = EdgeTotal - LastLeftEncoderValue;
    LastLeftEncoderValue = EdgeTotal;
}

void __ISR(_INPUT_CAPTURE_2_VECTOR, IPL6SOFT) IC2_ISR(void) { // right                                                                                                                                                                                                                                                                     
    // Get edge value
    uint16_t CurrentEdgeValue = IC2BUF;
    
    // Clear buffer
    while (IC2CONbits.ICBNE) {
        CurrentEdgeValue = IC2BUF;
    }
    // Clear mask
    IFS0CLR = _IFS0_IC2IF_MASK;
    
    if (IFS0bits.T2IF == 1 && (CurrentEdgeValue < 0x8000)) {
        RollOverCount++;
        IFS0CLR = _IFS0_T2IF_MASK;
    }    
        
    uint32_t EdgeTotal = ((uint32_t)RollOverCount << 16 | CurrentEdgeValue);
    RightEncoderPeriod = EdgeTotal - LastRightEncoderValue;
    LastRightEncoderValue = EdgeTotal;

}
// RPM Control
void __ISR(_TIMER_4_VECTOR, IPL2SOFT) T4_ISR(void) {
    IFS0CLR = _IFS0_T4IF_MASK;

    if (LeftEncoderPeriod == 0) {
        LeftRPM = 0;
    } else if (LeftEncoderPeriod == LastLeftEncoderPeriod) {
        LeftNoChangeCount++;
        if (LeftNoChangeCount > 100) {
            LeftEncoderPeriod = 0;
            LeftRPM = 0;
            LeftNoChangeCount = 0;
        }
    } else {
        LeftRPM = RPM_CONVERSION / LeftEncoderPeriod;
    }
    LastLeftEncoderPeriod = LeftEncoderPeriod;
    
    if (RightEncoderPeriod == 0) {
        RightRPM = 0;
    } else if (RightEncoderPeriod == LastRightEncoderPeriod) {
        RightNoChangeCount++;
        if (RightNoChangeCount > 100) {
            RightEncoderPeriod = 0;
            RightRPM = 0;
            RightNoChangeCount = 0;
        }
    } else {
        RightRPM = RPM_CONVERSION / RightEncoderPeriod;
    }
    LastRightEncoderPeriod = RightEncoderPeriod;
    
    Control(LeftRPM, RightRPM);
}

/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

