/****************************************************************************
 Module
   TemplateService.c

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
#include "BeaconService.h"
#include "PIC32_SPI_HAL.h"
#include "MotorSM.h"
#include "dbprintf.h"
#include "SPIFollowerService.h"

#include <xc.h>
#include <sys/attribs.h>
/*----------------------------- Module Defines ----------------------------*/
#define TICKS_PER_MS 2500 //assume prescale 8 pbclk

#define BEACON_G_PERIOD (BEACON_G_PERIOD_US)*(TICKS_PER_MS)/1000
#define BEACON_B_PERIOD (BEACON_B_PERIOD_US)*(TICKS_PER_MS)/1000
#define BEACON_R_PERIOD (BEACON_R_PERIOD_US)*(TICKS_PER_MS)/1000
#define BEACON_L_PERIOD (BEACON_L_PERIOD_US)*(TICKS_PER_MS)/1000

#define BEACON_TOLERANCE 50

#define BEACON_REQUIRED_COUNT 3 // consecutive periods to confirm detection



/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
void BeaconPinsInit(void);
void T2_Init(void);
void Beacon_ICRegs_Init(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint8_t CurrentState;

bool InitBeaconService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
  
  BeaconPinsInit(); // initialize beacon checking pins
  
  __builtin_disable_interrupts();
  
  T2_Init(); // initialize timer2
  
  Beacon_ICRegs_Init(); // initialize input capture registers for beacon detection
  
  __builtin_enable_interrupts();
  
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

bool PostBeaconService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunBeaconService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
  switch (CurrentState)
  {
      case InitBeaconState:
      {
          if (ThisEvent.EventType == ES_INIT)
          {
              CurrentState = NotLooking;
          }
          break;
      }
      case NotLooking:
      {
          if (ThisEvent.EventType == ES_LOOK_FOR_BEACON)
          {
                        DB_printf("Looking for beacon now \n");
              CurrentState = Looking;
          }
          break;
      }      
      case Looking:
      {
          switch (ThisEvent.EventType)
          {
              case ES_SIDEBEACON_DETECT:
              {
                  switch (ThisEvent.EventParam)
                  {
                      case BEACON_G:
                      {
                        DB_printf("Beacon G seen \n");
                          break;
                      }
                      case BEACON_B:
                      {
                        DB_printf("Beacon G seen \n");
                          break;
                      }
                      
                      // Using R and L beacons to determine side
                      case BEACON_R:
                      {
                        DB_printf("Beacon R seen \n");
                        UpdateStatus(BEACON_R_DETECT);
                        break;
                      }
                      case BEACON_L:
                      {
                        DB_printf("Beacon L seen \n");
                        UpdateStatus(BEACON_L_DETECT);
                        break;
                      }
                  }
                  break;
              }
              
          }
          CurrentState = NotLooking;
          break;
      }
  }
  return ReturnEvent;
}


void __ISR(_INPUT_CAPTURE_3_VECTOR, IPL5SOFT) IC3_ISR(void)
{
    
    IFS0CLR = _IFS0_IC3IF_MASK; // clear flag
    static uint32_t lastTimeVal = 0;
    static uint8_t BeaconCount = 0;
    static BeaconType_t lastBeacon = BEACON_NONE;
    static uint32_t CurrentPeriod = 0;
    while (IC3CONbits.ICBNE) // Clear buffer
    {
        uint32_t currentTimeVal = IC3BUF;
        uint32_t dt = currentTimeVal - lastTimeVal;
        lastTimeVal = currentTimeVal;
        CurrentPeriod = dt;
    }
        BeaconType_t detectedBeacon = BEACON_NONE;

        if (CurrentPeriod >= BEACON_G_PERIOD - BEACON_TOLERANCE && CurrentPeriod <= BEACON_G_PERIOD + BEACON_TOLERANCE)
        {
            detectedBeacon = BEACON_G;
        }
        else if (CurrentPeriod >= BEACON_B_PERIOD - BEACON_TOLERANCE && CurrentPeriod <= BEACON_B_PERIOD + BEACON_TOLERANCE)
        {
            detectedBeacon = BEACON_B;
        }
        else if (CurrentPeriod >= BEACON_R_PERIOD - BEACON_TOLERANCE && CurrentPeriod <= BEACON_R_PERIOD + BEACON_TOLERANCE)
        {
            detectedBeacon = BEACON_R;
        }
        else if (CurrentPeriod >= BEACON_L_PERIOD - BEACON_TOLERANCE && CurrentPeriod <= BEACON_L_PERIOD + BEACON_TOLERANCE)
        {
            detectedBeacon = BEACON_L;
        }

        if (detectedBeacon != BEACON_NONE)
        {
            if (detectedBeacon == lastBeacon)
            {
                BeaconCount++;
            }
            else
            {
                lastBeacon = detectedBeacon;
                BeaconCount = 1; // first detection of new beacon
            }
        }
        else // Invalid period, reset
        {
            lastBeacon = detectedBeacon;
            BeaconCount = 0;
        }
    
    
    if (BeaconCount == BEACON_REQUIRED_COUNT)
    {
        ES_Event_t ThisEvent;
        ThisEvent.EventType = ES_SIDEBEACON_DETECT;
        ThisEvent.EventParam = lastBeacon; // pass which beacon was detected

        PostBeaconService(ThisEvent);
        
        BeaconCount = 0;
        lastBeacon = BEACON_NONE;
    }

}


/***************************************************************************
 private functions
 ***************************************************************************/

void BeaconPinsInit(void)
{
    // back/side beacon: IC3 on RB8
    TRISBbits.TRISB8 = 1; //input
    // no analog for b8
    IC3R = 0b0100; // pps for IC3 - RB8
    
    // front beacon: IC4 on RB15
    TRISBbits.TRISB15 = 1; //input
    ANSELBbits.ANSB15 = 0; //disable analog
    IC4R = 0b0011; // pps for IC4 - RB15
}

void T2_Init(void)
{
    T2CONbits.ON = 0; // Timer2 off
    T2CONbits.TCS = 0; // PBClk as source
    T2CONbits.TCKPS = 0b011; // divide PBClk by 8
    TMR2 = 0; // Reset timer
    PR2 = 0xFFFF; // max period
    T2CONbits.ON = 1; // Timer2 on
}

void Beacon_ICRegs_Init(void)
{
    //IC3 INIT:
    IC3CONbits.ON = 0; // IC3 off
    IC3CONbits.ICTMR = 1; // Assign timer 2
    IC3CONbits.ICM = 0b011; // capture on rising edge
  
    while (IC3CONbits.ICBNE){volatile uint16_t dump = IC3BUF;} // empty FIFO
    
    IFS0CLR = _IFS0_IC3IF_MASK; // clear flag
    IEC0SET = _IEC0_IC3IE_MASK; // enable interrupt
    IPC3bits.IC3IP = 5; // priority 5
    IPC3bits.IC3IS = 1;  // subpriority 1
  
    IC3CONbits.ON = 1; // IC3 on
    
    //IC4 INIT:
    IC4CONbits.ON = 0; // IC4 off
    IC4CONbits.ICTMR = 1; // Assign timer 2
    IC4CONbits.ICM = 0b011; // capture on rising edge
  
    while (IC4CONbits.ICBNE){volatile uint16_t dump = IC4BUF;} // empty FIFO
    
    IFS0CLR = _IFS0_IC4IF_MASK; // clear flag
    IEC0SET = _IEC0_IC4IE_MASK; // enable interrupt
    IPC4bits.IC4IP = 5; // priority 5
    IPC4bits.IC4IS = 0;  // subpriority 0
  
    IC3CONbits.ON = 1; // IC3 on
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

