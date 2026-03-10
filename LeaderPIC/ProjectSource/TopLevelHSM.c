/****************************************************************************
 Module
   TopHSMTemplate.c

 Revision
   2.0.1

 Description
   This is a template for the top level Hierarchical state machine

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/20/17 14:30 jec      updated to remove sample of consuming an event. We 
                         always want to return ES_NO_EVENT at the top level 
                         unless there is a non-recoverable error at the 
                         framework level
 02/03/16 15:27 jec      updated comments to reflect small changes made in '14 & '15
                         converted unsigned char to bool where appropriate
                         spelling changes on true (was True) to match standard
                         removed local var used for debugger visibility in 'C32
                         removed Microwave specific code and replaced with generic
 02/08/12 01:39 jec      converted from MW_MasterMachine.c
 02/06/12 22:02 jec      converted to Gen 2 Events and Services Framework
 02/13/10 11:54 jec      converted During functions to return Event_t
                         so that they match the template
 02/21/07 17:04 jec      converted to pass Event_t to Start...()
 02/20/07 21:37 jec      converted to use enumerated type for events
 02/21/05 15:03 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "TopLevelHSM.h"
#include "PIC32_SPI_HAL.h"
#include "SPILeaderService.h"
#include "GameHSM.h"
#include "ServoService.h"
#include "OrientService.h"
#include "dbprintf.h"

#include "dbprintf.h"
#include <xc.h>
#include <sys/attribs.h>

/*----------------------------- Module Defines ----------------------------*/
#define ORIENT_PERIOD 5000
#define LED_OUTPUT LATBbits.LATB9
/*---------------------------- Module Functions ---------------------------*/
static ES_Event_t DuringGameState( ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, you could get
// away without it
static MasterState_t CurrentState;
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

static uint8_t Counter;
static ServoEventParam_t Side;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMasterSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     boolean, False if error in initialization, True otherwise

 Description
     Saves away the priority,  and starts
     the top level state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:06
****************************************************************************/
bool InitMasterSM ( uint8_t Priority )
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;  // save our priority

  ThisEvent.EventType = ES_ENTRY;
  // Start the Master State machine
//  TRISBbits.TRISB8 = 1;   // RB8 as input
  Counter = 0;
  TRISBbits.TRISB9 = 0; //LED output
  LED_OUTPUT = 0;

  
  DB_printf("\n\n\n");
  DB_printf("Init TopLevelHSM \n");
  
  
  StartMasterSM( ThisEvent );

  return true;
}

/****************************************************************************
 Function
     PostMasterSM

 Parameters
     ES_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the post operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostMasterSM( ES_Event_t ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMasterSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   the run function for the top level state machine 
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 02/06/12, 22:09
****************************************************************************/
ES_Event_t RunMasterSM( ES_Event_t CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   MasterState_t NextState = CurrentState;
   ES_Event_t EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event_t ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error
   
   
   if(CurrentEvent.EventType == ES_TIMEOUT && CurrentEvent.EventParam == GAME_TIMER){
        ES_Timer_InitTimer(GAME_TIMER, 1000);
        Counter++;
        
    }
        
    switch ( CurrentState )
   {
        /************************ IDLE ************************/
       case IDLE_STATE:
        if (CurrentEvent.EventType == START)
        {
            DB_printf("button triggers \n");
            ES_Timer_InitTimer(GAME_TIMER, 1000);
            LED_OUTPUT = 1;
            //NextState = GAME_STATE; // for test
            NextState = ORIENTATION_STATE;
            CurrentEvent.EventType = ES_LEADER_SEND;
            CurrentEvent.EventParam = BEACON_TURN_CW;
            PostSPILeaderService(CurrentEvent);
            ES_Timer_InitTimer(ORIENT_TIMER, ORIENT_PERIOD*2);
            
            MakeTransition = true;
        }
        break;

        /********************** ORIENTATION ********************/
        case ORIENTATION_STATE:
            switch(CurrentEvent.EventType)
            {
                case ES_TIMEOUT:
                    if(CurrentEvent.EventParam == ORIENT_TIMER){
                        CurrentEvent.EventType = ES_LEADER_SEND;
                        CurrentEvent.EventParam = BEACON_TURN_CCW;
                        PostSPILeaderService(CurrentEvent);
                    }
                    break;
                    
                        
                case ES_BEACON_DETECT:
                    switch(CurrentEvent.EventParam)
                    {
                        case ORIENTATION_DONE_G:
                        {
                            DB_printf("on green side");
                            CurrentEvent.EventType = SERVO_INDICATOR;
                            CurrentEvent.EventParam = Green;
                            PostServoService(CurrentEvent);
                            
                            // control servo to point to G
//                            CurrentEvent.EventType = ES_LEADER_SEND;
//                            CurrentEvent.EventParam = ROBOT_STOP;
//                            PostSPILeaderService(CurrentEvent); 
                            break;
                        }
                        case ORIENTATION_DONE_B:
                        {
                            DB_printf("on blue side");
                            CurrentEvent.EventType = SERVO_INDICATOR;
                            CurrentEvent.EventParam = Blue;
                            PostServoService(CurrentEvent);
                            
                            
                            
                            // control servo to point to B
//                            CurrentEvent.EventType = ES_LEADER_SEND;
//                            CurrentEvent.EventParam = ROBOT_STOP;
//                            PostSPILeaderService(CurrentEvent); 
                            break;
                        }
                        
                    }
                        ES_Timer_StopTimer(ORIENT_TIMER);
                        NextState = GAME_STATE;
                        MakeTransition = true;
                    break;
            }
            break;
        
                

                
         /************************ GAME *************************/
        case GAME_STATE :

           CurrentEvent = DuringGameState(CurrentEvent);
                if(Counter == 138){
                    LED_OUTPUT = 0;
                    CurrentEvent.EventType = ES_LEADER_SEND;
                    CurrentEvent.EventParam = ROBOT_STOP;
                    PostSPILeaderService(CurrentEvent); 
                    NextState = IDLE_STATE;
                    MakeTransition = true;
                }
                break;

           break;
        
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunMasterSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       // Execute entry function for new state
       // this defaults to ES_ENTRY
       RunMasterSM(EntryEventKind);
     }
   // in the absence of an error the top level state machine should
   // always return ES_NO_EVENT, which we initialized at the top of func
   return(ReturnEvent);
}
/****************************************************************************
 Function
     StartMasterSM

 Parameters
     ES_Event CurrentEvent

 Returns
     nothing

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:15
****************************************************************************/
void StartMasterSM ( ES_Event_t CurrentEvent )
{
  DB_printf("Starting master");
  // if there is more than 1 state to the top level machine you will need 
  // to initialize the state variable
  CurrentState = IDLE_STATE;
  // now we need to let the Run function init the lower level state machines
  // use LocalEvent to keep the compiler from complaining about unused var
  RunMasterSM(CurrentEvent);
  return;
}

/****************************************************************************
 Function
     QueryTopHSMTemplateSM

 Parameters
     None

 Returns
     MasterState_t  The current state of the Top Level Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/05/22, 10:30AM
****************************************************************************/
//MasterState_t  QueryGameSM ( void )
//{
//   return(CurrentState);
//}

/***************************************************************************
 private functions
 ***************************************************************************/
static ES_Event_t DuringGameState( ES_Event_t Event )
{
    ES_Event_t ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine

        // after that start any lower level machines that run in this state
        StartGameSM(Event);
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunGameSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        ReturnEvent = RunGameSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}
