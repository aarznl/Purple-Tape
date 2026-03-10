/****************************************************************************
 Module
   MotorService.c

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
#include "MotorService.h"
#include "CommandService.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/

#define AD_MAX 1023.0

#define PERIOD 249

#define OC1_CW RPB3R
#define OC1_CCW RPB4R
#define OC2_CW RPB8R
#define OC2_CCW RPB11R

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static bool DirectionCW;

static MotorServiceState_t CurrentState;

static uint8_t LastCmd = 0xAA;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMotorService

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
bool InitMotorService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  
  CurrentState = InitMotorState;
  /********************************************
   in here you write your initialization code
   *******************************************/
  // Set DC+ pin as digital output
  TRISAbits.TRISA0 = 0;
  ANSELAbits.ANSA0 = 0;

  // Set DC- pin as digital output, and initialize to low
  TRISBbits.TRISB3 = 0;
  ANSELBbits.ANSB3 = 0;
  TRISBbits.TRISB4 = 0;
  
  TRISBbits.TRISB8 = 0;
  TRISBbits.TRISB11 = 0;
  
  T3CONbits.ON = 0; // Timer3 off
  T3CONbits.TCS = 0; // PBClk as source
  T3CONbits.TCKPS = 0b011; // divide PBClk by 8
  TMR3 = 0;
  PR3 = PERIOD; // period for 10kHz
  
  OC1CONbits.ON = 0; // OC1 off
  OC1CON = 0;
  OC1CONbits.OCTSEL = 1; // Assign timer 3
  OC1CONbits.OCM = 0b110; // PWM mode, edge-aligned
  OC1RS = PERIOD*0.8; // duty cycle 0
  OC1R = OC1RS;
  OC1CONbits.ON = 1; // OC1 on
  
  OC2CONbits.ON = 0; // OC2 off
  OC2CON = 0;
  OC2CONbits.OCTSEL = 1; // Assign timer 3
  OC2CONbits.OCM = 0b110; // PWM mode, edge-aligned
  OC2RS = PERIOD*0.8; // duty cycle 0
  OC2R = OC2RS;
  OC2CONbits.ON = 1; // OC2 on
  
  T3CONbits.ON = 1; // Timer3 on
  
  TRISBbits.TRISB14 = 1;
  ANSELBbits.ANSB14 = 0;
  
  DirectionCW = PORTBbits.RB14;
  
  // Determine starting direction
  //changeDirection(DirectionCW);
  
  // Switch OC1 line
  OC1_CW = 0;
  OC1_CCW = 0b0101;
  OC2_CW = 0;
  OC2_CCW = 0b0101;
  // Set drive line high
  DB_printf("For the RIGHT motor: Press 'f' to drive backward. Press 'd' to drive forward.\n");
  DB_printf("For the LEFT motor: Press 'j' to drive backward. Press 'k' to drive forward.\n");
  
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
     PostMotorService

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
bool PostMotorService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMotorService

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
ES_Event_t RunMotorService(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
    /********************************************
     in here you write your service code
     *******************************************/
    switch (CurrentState)
    {
        case InitMotorState:
        {
            if (ThisEvent.EventType == ES_INIT)
            {
                CurrentState = MotorWaiting4Command;
            }
            break;
        }
        case MotorWaiting4Command:
        {
            switch (ThisEvent.EventType)
            {
                case ES_NEW_CMD:
                {
                    CurrentState = MotorRunningCommand;
                    if (ThisEvent.EventParam != LastCmd)
                    {
                    // DO SOMETHING
                        if (ThisEvent.EventParam == 0x20) {
                            CurrentState = Looking4Beacon;
                        }                        
                        DB_printf("New command: %u\n",ThisEvent.EventParam);
                        ExecuteCommand(ThisEvent.EventParam);
                        LastCmd = ThisEvent.EventParam;
                    }
//                    DB_printf("Command done in Motor Service.\n");
                    ES_Event_t CommandDone;
                    CommandDone.EventType = ES_CMD_DONE;
                    PostCommandService(CommandDone);
                    PostMotorService(CommandDone);
                    break;
                }
                
                default:
                    break;
            }
            break;
        }
        case MotorRunningCommand:
        {
            switch (ThisEvent.EventType)
            {
                case ES_CMD_DONE:
                {
                    CurrentState = MotorWaiting4Command;
                    break;
                }
                
            }
                default:
                    break;            
            break;
        }
        case Looking4Beacon:
        {
            switch (ThisEvent.EventType)
            {
                case ES_NEW_CMD:
                {
                    if (ThisEvent.EventParam != LastCmd)
                    {
                    // DO SOMETHING
                        CurrentState = MotorRunningCommand;
                        DB_printf("New command: %u\n",ThisEvent.EventParam);
                        ExecuteCommand(ThisEvent.EventParam);
                        LastCmd = ThisEvent.EventParam;
                    }
//                   DB_printf("Command done in Motor Service.\n");
                    ES_Event_t CommandDone;
                    CommandDone.EventType = ES_CMD_DONE;
                    PostCommandService(CommandDone);
                    PostMotorService(CommandDone);
                    break;
                }
                /*
                case ES_CMD_DONE:
                {
                    CurrentState = MotorWaiting4Command;
                    break;
                }
                */
                case ES_BEACON_DETECT:
                {
                    // STOP THE MOTORS
                    UpdateRPM(100,0); // stop motor 1
                    UpdateRPM(100,1); // stop motor 2
                    DB_printf("beacon detected. stopping turning!\n");
                    CurrentState = MotorWaiting4Command;
                    break;
                }
                default:
                    break;                
            }
            break;
        }
        case TapeDetected:
        {
            // don't do anything b/c done
            break;
        }
    }
    // these next few will happen for ALL STATES
        switch (ThisEvent.EventType)
        {
            
            case ES_NEW_KEY:
            {
                switch (ThisEvent.EventParam)
                {            
                    case 'f':
                        changeDirection(0,0);
                        break;
                    case 'd':
                        changeDirection(1,0);
                        break;
                    case 'j':
                        changeDirection(0,1);
                        break;
                    case 'k':
                        changeDirection(1,1);
                        break;
                    default:
                        break;
                }
                break;
            }

            case ES_TIMEOUT:
                {
                    DB_printf("timed out... ");
                    if (ThisEvent.EventParam == 12)
                    {
                        UpdateRPM(100,0); // stop motor 1
                        UpdateRPM(100,1); // stop motor 2
                        DB_printf("position reached. stopping turning!\n");
                    }
                    
                    break;
                }
            case ES_TAPE_DETECT:
            {
                if (CurrentState != TapeDetected) {
                    CurrentState = TapeDetected;
                    UpdateRPM(100,0); // stop motor 1
                    UpdateRPM(100,1); // stop motor 2
                    DB_printf("line detected. stop everything!\n");
                }
                // POST AN EVENT TO THE COMMAND SERVICE TO SAY WE'RE DONE
                break;
            }

            default:
                break;
        }
    
  
    return ReturnEvent;
}
void UpdateRPM(float DutyValue, bool whichMotor)
{
    // Calculate new duty pct 
    float dutyPct = 1.0 - ((float)DutyValue/100.0);
    // Calculate new period value
    uint32_t periodVal = PERIOD*dutyPct;
    // Update OC1 line
    if (whichMotor == 1)
    {
        OC1RS = periodVal;
    }
    else
    {
        OC2RS = periodVal;
    }
}

/***************************************************************************
 private functions
 ***************************************************************************/

static void changeDirection(bool whichDirection, bool whichMotor){
    if (whichMotor == 1) {
        if (whichDirection == 1) {
            OC1_CW = 0;
            OC1_CCW = 0b0101;                                                                                                                           
        } else {                                                                                                                            
            OC1_CCW = 0;
            OC1_CW = 0b0101;
        }
    }
    else {
        if (whichDirection == 1) {
            OC2_CW = 0;
            OC2_CCW = 0b0101;                                                                                                                           
        } else {                                                                             
            OC2_CCW = 0;
            OC2_CW = 0b0101;
        }
    }
}

void ExecuteCommand (uint8_t cmd)
{
    switch (cmd)
    {
        case 0x00:
        {
            DB_printf("Stop!\n");
            UpdateRPM(100,0); // stop motor 1
            UpdateRPM(100,1); // stop motor 2
            break;
        }
        case 0x02:
        {
            DB_printf("Rotate clockwise by 90 deg.\n");
            changeDirection(0,0); // right motor backward
            changeDirection(1,1); // left motor forward
            UpdateRPM(0,0); // drive motor 1
            UpdateRPM(0,1); // drive motor 2
            ES_Timer_InitTimer(ANGLE_TIMER,2000);

            break;
        }
        case 0x03:
        {
            DB_printf("Rotate clockwise by 45 deg.\n");
            changeDirection(0,0); // right motor backward
            changeDirection(1,1); // left motor forward
            UpdateRPM(0,0); // drive motor 1
            UpdateRPM(0,1); // drive motor 2
            ES_Timer_InitTimer(ANGLE_TIMER,1000);
            break;
        }
        case 0x04:
        {
            DB_printf("Rotate counterclockwise by 90 deg.\n");
            changeDirection(1,0); // right motor forward
            changeDirection(0,1); // left motor backwards
            UpdateRPM(0,0); // drive motor 1
            UpdateRPM(0,1); // drive motor 2
            ES_Timer_InitTimer(ANGLE_TIMER,2000);
            break;
        }
        case 0x05:
        {
            DB_printf("Rotate counterclockwise by 45 deg.\n");
            changeDirection(1,0); // right motor forward
            changeDirection(0,1); // left motor backwards
            UpdateRPM(0,0); // drive motor 1
            UpdateRPM(0,1); // drive motor 2
            ES_Timer_InitTimer(ANGLE_TIMER,1000);
            break;
        }
        case 0x08:
        {
            DB_printf("Drive forward half-speed.\n");
            changeDirection(1,0); // right motor forwards
            changeDirection(1,1); // left motor forwards
            UpdateRPM(50,0); // drive motor 1
            UpdateRPM(50,1); // drive motor 2
            break;
        }
        case 0x09:
        {
            DB_printf("Drive forward full-speed.\n");
            changeDirection(1,0); // right motor forwards
            changeDirection(1,1); // left motor forwards
            UpdateRPM(0,0); // drive motor 1
            UpdateRPM(0,1); // drive motor 2
            break;
        }
        case 0x10:
        {
            DB_printf("Drive reverse half-speed.\n");
            changeDirection(0,0); // right motor backward
            changeDirection(0,1); // left motor backwards
            UpdateRPM(50,0); // drive motor 1
            UpdateRPM(50,1); // drive motor 2
            break;
        }
        case 0x11:
        {
            DB_printf("Drive reverse full-speed.\n");
            changeDirection(0,0); // right motor backward
            changeDirection(0,1); // left motor backwards
            UpdateRPM(0,0); // drive motor 1
            UpdateRPM(0,1); // drive motor 2
            break;
        }
        case 0x20:
        {
            DB_printf("Align with beacon.\n");
            changeDirection(1,0); // right motor forward
            changeDirection(0,1); // left motor backwards
            UpdateRPM(0,0); // drive motor 1
            UpdateRPM(0,1); // drive motor 2
            break;
        }
        case 0x40:
        {
            DB_printf("Drive forward until tape detected.\n");
            changeDirection(1,0); // right motor forwards
            changeDirection(1,1); // left motor forwards
            UpdateRPM(0,0); // drive motor 1
            UpdateRPM(0,1); // drive motor 2
            break;
        }
        default:
            break;        
    }
}

MotorServiceState_t QueryMotorService(void){
    return CurrentState;
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

