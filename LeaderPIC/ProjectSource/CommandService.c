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
#include "CommandService.h"
#include "PIC32_SPI_HAL.h"
#include "MotorService.h"
#include "dbprintf.h"

#include <xc.h>
#include <sys/attribs.h>
/*----------------------------- Module Defines ----------------------------*/
#define BEACON_PERIOD 1750
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;


static CommandState_t CurrentState;
static uint8_t command = 0;
static uint8_t SentData = 0xAA;

static uint8_t LastCmd = 0xAA;



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
bool InitCommandService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
  InitSPI();
  CurrentState = Waiting4Command;
  ES_Timer_InitTimer(QUERY_TIMER, 500); 
  
  
  // initialize beacon checking pin
  TRISBbits.TRISB13 = 1;
  ANSELBbits.ANSB13 = 0;
  
  // initialize line checking pin
  TRISBbits.TRISB2 = 1;
  ANSELBbits.ANSB2 = 0;
  
  
  IC1R = 0b0011;
  
  T2CONbits.ON = 0; // Timer2 off
  T2CONbits.TCS = 0; // PBClk as source
  T2CONbits.TCKPS = 0b011; // divide PBClk by 8
  TMR2 = 0; // Reset timer
  PR2 = 0xFFFF; // max period
  T2CONbits.ON = 1; // Timer2 on
  
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
bool PostCommandService(ES_Event_t ThisEvent)
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
ES_Event_t RunCommandService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
  switch (CurrentState)
  {
    case Waiting4Command:
      switch(ThisEvent.EventType)
      {
        case ES_TIMEOUT:
          if(ThisEvent.EventParam == QUERY_TIMER){
//            DB_printf("command1 = %u", command);
            SPIOperate_SPI1_Send8Wait(SentData);
            command = SPIOperate_ReadData(SPI_SPI1);
//            DB_printf("command2 = %u\n", command);
//            DB_printf("data = %u\n", SentData);
            if(command != 0xFF){
//              DB_printf("command received: %u\n", command);
              CurrentState = RunningCommand;
//              if (command != LastCmd) // only post if it's a new command
//              {
//                  DB_printf("Posting command to motor.\n");
                  ES_Event_t SendingCommand;
                  SendingCommand.EventType = ES_NEW_CMD;
                  SendingCommand.EventParam = command;
                  PostMotorService(SendingCommand);
                  
//                  DB_printf("Last Command: %u\n",LastCmd);
//              }
            }
            else{
//                DB_printf("re-init timer.\n");
              ES_Timer_InitTimer(QUERY_TIMER, 500); 
            }
          }
        break;

        case ES_TAPE_DETECT: // done! yay
          CurrentState = LineDetected;
          DB_printf("success!\n");
        break;
      }
      
    break;

    case RunningCommand:
      switch(ThisEvent.EventType)
      {
        // case ES_NEW_CMD:
        //   if(ThisEvent.EventParam == 0x20){
        //     CurrentState = Looking4Beacon;

        //   }

        case ES_CMD_DONE:
//          DB_printf("here");
          CurrentState = Waiting4Command;
          ES_Timer_InitTimer(QUERY_TIMER, 500); 
        break;

        case ES_TAPE_DETECT:  // done! yay
          CurrentState = LineDetected;
          DB_printf("success!\n");
        break;
        
        default:
        break;
      }
      break;
      
    case LineDetected:
        
      break;

      
    default:
      break;

  }
  return ReturnEvent;
}

/*
bool Check4Beacon(void)
{
    uint8_t CurrentBeaconStatus = PORTBbits.RB15;
    bool ReturnVal = false;
    static uint8_t BeaconDetectCount = 0;

    //if (CurrentBeaconStatus != LastBeaconStatus) {
        if (CurrentBeaconStatus == 1) {
            //DB_printf("up!");
            if (BeaconDetectCount > 50) {
                ES_Event_t ThisEvent;
                ThisEvent.EventType = ES_BEACON_DETECT;
                PostMotorService(ThisEvent);
                BeaconDetectCount = 0;
            } else {
                BeaconDetectCount++;
            }
            ReturnVal = true;
        }
    //}
    LastBeaconStatus = CurrentBeaconStatus;
    return ReturnVal;
}
*/

bool Check4Line(void)
{
    uint8_t CurrentLineStatus = PORTBbits.RB2;
    bool ReturnVal = false;
    
    static uint8_t LineDetectCount = 0;
    if (CurrentLineStatus == 1){
        if (LineDetectCount > 50) {
            ES_Event_t ThisEvent;
            ThisEvent.EventType = ES_TAPE_DETECT;
            ES_PostAll(ThisEvent);
            LineDetectCount = 0;
        } else {
            LineDetectCount++;
        }
        ReturnVal = true;

    }
    return ReturnVal;    
}

void __ISR(_INPUT_CAPTURE_1_VECTOR, IPL5SOFT) IC1_ISR(void) {
    IFS0CLR = _IFS0_IC1IF_MASK; // clear flag
    static uint32_t lastTimeVal = 0;
    static uint8_t BeaconCount = 0;
    static uint32_t CurrentPeriod = 0;
    // Clear buffer
    while (IC1CONbits.ICBNE) {
        uint32_t currentTimeVal = IC1BUF;
        // Calculate period of encoder
        uint32_t dt = currentTimeVal-lastTimeVal;
        lastTimeVal = currentTimeVal;
        
        CurrentPeriod = dt;
    }
        // Update global value
    if (CurrentPeriod < BEACON_PERIOD+50 && CurrentPeriod > BEACON_PERIOD-50) {
        BeaconCount++;
    } else if (CurrentPeriod <  BEACON_PERIOD) {
        //BeaconCount = 0;
    }
    if (BeaconCount > 3) {
        ES_Event_t ThisEvent;
        ThisEvent.EventType = ES_BEACON_DETECT;
        PostMotorService(ThisEvent);
        BeaconCount = 0;
    }
}


/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

