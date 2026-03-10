/****************************************************************************
 Module
   ADService.c

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
#include "ADService.h"
#include "PIC32_AD_Lib.h"
#include "MotorService.h"
#include "EncoderService.h"
#include "dbprintf.h"
                                                                                                       
/*----------------------------- Module Defines ----------------------------*/
#define UPDATE_INTERVAL 100

#define INTERVAL_MAX 20

#define AD_MAX 1023.0
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static uint32_t LastPotVal[1];

static uint16_t SendPotVal;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemplateService

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
bool InitADService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
  
  // initialize potentiometer port
  ANSELBbits.ANSB2 = 1;
  TRISBbits.TRISB2 = 1;
  
  // Set up AD pin
  ADC_ConfigAutoScan(BIT4HI);
  ADC_MultiRead(LastPotVal);
  // Set initial speed to maximum
  
  // Start ADService and StepService timers
  ES_Timer_InitTimer(AD_UPDATE_TIMER, UPDATE_INTERVAL);
  // post the initial transition event
  
  clrScrn();
  puts("\rStarting up services \r");
  
  
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
     PostTemplateService

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
bool PostADService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemplateService

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
ES_Event_t RunADService(ES_Event_t ThisEvent)
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
      if (ThisEvent.EventParam == AD_UPDATE_TIMER)
      {
        // Update speed of motor
        ChangeSpeedVal();
        
        // Create update speed event
        ES_Event_t UpdateEvent;
        UpdateEvent.EventType = ES_UPDATE_TARGET;
        UpdateEvent.EventParam = SendPotVal;
        
        // Post event to motor service
        PostEncoderService(UpdateEvent);
        
        // Reset timer
        ES_Timer_InitTimer(AD_UPDATE_TIMER, UPDATE_INTERVAL);
      }
      break;

    default:
      break;
  }


  return ReturnEvent;
}

// Function for StepService to query speed of motor
uint16_t GetTimeInterval(void)
{
    return SendPotVal;
}

/***************************************************************************
 private functions
 ***************************************************************************/

// Function to update speed value based on AD value
static void ChangeSpeedVal(void)
{
    // Read in current analog pin value
    uint32_t CurrentPotVal[1];
    ADC_MultiRead(CurrentPotVal);
    
    // Get new value for speed update
    SendPotVal = CurrentPotVal[0];
        
    // Update last potentiometer value
    LastPotVal[0] = CurrentPotVal[0];
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

