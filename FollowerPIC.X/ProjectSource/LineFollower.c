/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "LineFollower.h"
#include "PIC32_AD_Lib.h"
#include "SPIFollowerService.h"
#include "MotorSM.h"
#include "dbprintf.h"

#include <xc.h>
#include <sys/attribs.h>
/*----------------------------- Module Defines ----------------------------*/
#define LINESENSE_PERIOD 100

//defines for auto scan:
#define RB2_ANx (1<<4) // front left
#define RB3_ANx (1<<5) // front middle
#define RB12_ANx (1<<12) // front right
#define RB13_ANx (1<<11) // back left
#define RB15_ANx (1<<9) // back right

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
void InitLineSensors(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static bool PrintLineVals = false;

uint32_t LineSensorValues[5];

static FollowState_t CurrentState;

bool InitLineFollower(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
  
  InitLineSensors();
  uint16_t AutoScanBitMask = RB2_ANx | RB3_ANx | RB12_ANx | RB13_ANx | RB15_ANx;
  ADC_ConfigAutoScan(AutoScanBitMask);
  
  ES_Timer_InitTimer(LINESENSE_TIMER, LINESENSE_PERIOD);
  CurrentState = Look4Tape;
  
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

bool PostLineFollower(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunLineFollower(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
//  switch (ThisEvent.EventType)
//  {
//      case ES_TIMEOUT:
//      {
//          if (ThisEvent.EventParam == LINESENSE_TIMER)
//          {
//              ADC_MultiRead(LineSensorValues); // read values
//              if (PrintLineVals == true) // only print if we've enabled it
//              {
//                  DB_printf("Sensor Readings: ");
//                  for (uint8_t i = 0; i < 5; i++)
//                  {
////                    DB_printf("%d, ", LineSensorValues[i]);
//                      DB_printf("%d, ", LineSensorValues[1]);
//                  }
//                  DB_printf("\n");
//              }
//              ES_Timer_InitTimer(LINESENSE_TIMER, LINESENSE_PERIOD);
//          }
//          break;
//      }
//  }
    switch(CurrentState)
    {
        case Look4Tape:
        {
            switch (ThisEvent.EventType)
            {
                case ES_TIMEOUT:
                {
                    if (ThisEvent.EventParam == LINESENSE_TIMER)
                    {
                        ADC_MultiRead(LineSensorValues); // read values
                        if(LineSensorValues[1] >= 0 && LineSensorValues[1] <= 60){ // the front middle sees the tape
                            CurrentState = FollowingTape;
                            
                            ES_Event_t NewEvent;
                            NewEvent.EventType = ES_TAPE_DETECT;
                            PostSPIFollowerService(NewEvent);
                        }
                        ES_Timer_InitTimer(LINESENSE_TIMER, LINESENSE_PERIOD);
                    }
                    break;
                }
            }
        }
        
        case FollowingTape:
        {
            switch (ThisEvent.EventType)
            {
                case ES_TIMEOUT:
                {
                    if (ThisEvent.EventParam == LINESENSE_TIMER)
                    {
                        ADC_MultiRead(LineSensorValues); // read values
                        if(LineSensorValues[0] >= 0 && LineSensorValues[0] <= 60){ // the front left sees the tape
                            // add duty cycle on right motor and decrease on left motor
                        }else if(LineSensorValues[2] >= 0 && LineSensorValues[2] <= 60){ // the front right sees the tape
                            // add duty cycle on left motor and decrease on right motor
                        }else if((LineSensorValues[0] >= 0 && LineSensorValues[0]) &&
                                (LineSensorValues[2] >= 0 && LineSensorValues[2] <= 60)){ // all three front see the tape
                            // turn CCW 90 degree
                        }
                        ES_Timer_InitTimer(LINESENSE_TIMER, LINESENSE_PERIOD);
                    }
                    break;
                }
            }
        }
    }
  return ReturnEvent;
}

void InitLineSensors(void)
{
    // line sensors must be on analog pins
    // sensor pins we chose: B2, B3, B12, B13, B15
    
    // SENSOR 1: B2
    TRISBbits.TRISB2 = 1; //configure as input
    ANSELBbits.ANSB2 = 1; //enable analog
    
    // SENSOR 2: B3
    TRISBbits.TRISB3 = 1; //configure as input
    ANSELBbits.ANSB3 = 1; //enable analog
    
    // SENSOR 3: B12
    TRISBbits.TRISB12 = 1; //configure as input
    ANSELBbits.ANSB12 = 1; //enable analog
    
    // SENSOR 4: B13
    TRISBbits.TRISB13 = 1; //configure as input
    ANSELBbits.ANSB13 = 1; //enable analog
    
    // SENSOR 5: B15
    TRISBbits.TRISB15 = 1; //configure as input
    ANSELBbits.ANSB15 = 1; //enable analog
}