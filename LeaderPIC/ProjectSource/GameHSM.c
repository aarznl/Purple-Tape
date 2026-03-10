/****************************************************************************
 Module
   HSMTemplate.c

 Revision
   2.0.1

 Description
   This is a template file for implementing state machines.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/27/17 09:48 jec      another correction to re-assign both CurrentEvent
                         and ReturnEvent to the result of the During function
                         this eliminates the need for the prior fix and allows
                         the during function to-remap an event that will be
                         processed at a higher level.
 02/20/17 10:14 jec      correction to Run function to correctly assign 
                         ReturnEvent in the situation where a lower level
                         machine consumed an event.
 02/03/16 12:38 jec      updated comments to reflect changes made in '14 & '15
                         converted unsigned char to bool where appropriate
                         spelling changes on true (was True) to match standard
                         removed local var used for debugger visibility in 'C32
                         commented out references to Start & RunLowerLevelSM so
                         that this can compile. 
 02/07/13 21:00 jec      corrections to return variable (should have been
                         ReturnEvent, not CurrentEvent) and several EV_xxx
                         event names that were left over from the old version
 02/08/12 09:56 jec      revisions for the Events and Services Framework Gen2
 02/13/10 14:29 jec      revised Start and run to add new kind of entry function
                         to make implementing history entry cleaner
 02/13/10 12:29 jec      added NewEvent local variable to During function and
                         comments about using either it or Event as the return
 02/11/10 15:54 jec      more revised comments, removing last comment in during
                         function that belongs in the run function
 02/09/10 17:21 jec      updated comments about internal transitions on During funtion
 02/18/09 10:14 jec      removed redundant call to RunLowerlevelSM in EV_Entry
                         processing in During function
 02/20/07 21:37 jec      converted to use enumerated type for events & states
 02/13/05 19:38 jec      added support for self-transitions, reworked
                         to eliminate repeated transition code
 02/11/05 16:54 jec      converted to implment hierarchy explicitly
 02/25/03 10:32 jec      converted to take a passed event parameter
 02/18/99 10:19 jec      built template from MasterMachine.c
 02/14/99 10:34 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "GameHSM.h"
 #include "DriveToDispenserFSM.h"
#include "DispenserToBucketFSM.h"
#include "TapeToBeaconFSM.h"
 #include "BeaconToTapeFMS.h"
 #include "TapeToDispenserFSM.h"

#include "dbprintf.h"
/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE DriveToDispenser

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringDriveToDispenser( ES_Event_t Event);
static ES_Event_t DuringDispenserToBucket( ES_Event_t Event);
static ES_Event_t DuringTapeToBeacon( ES_Event_t Event);
static ES_Event_t DuringBeaconToTape( ES_Event_t Event);
static ES_Event_t DuringTapeToDispenser( ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static GameState_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunTemplateSM

 Parameters
   ES_Event_t: the event to process

 Returns
   ES_Event_t: an event to return

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 2/11/05, 10:45AM
****************************************************************************/
ES_Event_t RunGameSM( ES_Event_t CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   GameState_t NextState = CurrentState;
   ES_Event_t EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event_t ReturnEvent = CurrentEvent; // assume we are not consuming event
   
   switch ( CurrentState )
   {
       case DriveToDispenser:
//           DB_printf("Game working ,event is %d", CurrentEvent.EventType);
            CurrentEvent = ReturnEvent = DuringDriveToDispenser(CurrentEvent);
//            DB_printf("CurrentEvent: %d\n", CurrentEvent);
            if (CurrentEvent.EventType == COLLECT_DONE)
            {
                DB_printf("Game processed ball collect done\n");
                NextState = DispenserToBucket;
                MakeTransition = true;
                ReturnEvent.EventType = ES_NO_EVENT;
            }
            break;


        case DispenserToBucket:
//            DB_printf("In DispenserToBucket\n");
           CurrentEvent = ReturnEvent = DuringDispenserToBucket(CurrentEvent);

           if (CurrentEvent.EventType == DISPENSE_DONE)
           {
               NextState = TapeToBeacon;
               MakeTransition = true;
               ReturnEvent.EventType = ES_NO_EVENT;
           }
            break;
//

       case TapeToBeacon:
//           DB_printf("In TapeToBeacon\n");
           CurrentEvent = ReturnEvent = DuringTapeToBeacon(CurrentEvent);

           if (CurrentEvent.EventType == DISPENSE_DONE)
           {
               NextState = BeaconToTape;
               MakeTransition = true;
               ReturnEvent.EventType = ES_NO_EVENT;
           }
           break;


       case BeaconToTape:
           DB_printf("In BeaconToTape\n");
           CurrentEvent = ReturnEvent = DuringBeaconToTape(CurrentEvent);

           if (CurrentEvent.EventType == DISPENSE_DONE)
           {
               NextState = TapeToDispenser;
               MakeTransition = true;
               ReturnEvent.EventType = ES_NO_EVENT;
           }
           break;

        case TapeToDispenser:
           DB_printf("In TapeToDispenser\n");
            CurrentEvent = ReturnEvent = DuringTapeToDispenser(CurrentEvent);
//            DB_printf("CurrentEvent: %d\n", CurrentEvent);
            if (CurrentEvent.EventType == COLLECT_DONE)
            {
                DB_printf("Game know collect done\n");
                NextState = DispenserToBucket;
                MakeTransition = true;
                ReturnEvent.EventType = ES_NO_EVENT;
            }
            break;
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunGameSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunGameSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartTemplateSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 2/18/99, 10:38AM
****************************************************************************/
void StartGameSM ( ES_Event_t CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
   // call the entry function (if any) for the ENTRY_STATE
   
   RunGameSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryTemplateSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
GameState_t QueryGameSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringDriveToDispenser(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event;

    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
       StartDriveToDispenserFSM(Event);
    }
    else if (Event.EventType == ES_EXIT)
    {
        RunDriveToDispenserFSM(Event);
    }
    else
    {
        ReturnEvent = RunDriveToDispenserFSM(Event);
//        DB_printf("ReturnEvent: %d\n", ReturnEvent);
    }

    return ReturnEvent;
}

static ES_Event_t DuringDispenserToBucket(ES_Event_t Event)
{
   ES_Event_t ReturnEvent = Event;

   if ((Event.EventType == ES_ENTRY) ||
       (Event.EventType == ES_ENTRY_HISTORY))
   {
       StartDispenserToBucketFSM(Event);
   }
   else if (Event.EventType == ES_EXIT)
   {
       RunDispenserToBucketFSM(Event);
   }
   else
   {
       ReturnEvent = RunDispenserToBucketFSM(Event);
   }

   return ReturnEvent;
}

static ES_Event_t DuringTapeToBeacon(ES_Event_t Event)
{
   ES_Event_t ReturnEvent = Event;

   if ((Event.EventType == ES_ENTRY) ||
       (Event.EventType == ES_ENTRY_HISTORY))
   {
       StartTapeToBeaconFSM(Event);
   }
   else if (Event.EventType == ES_EXIT)
   {
       RunTapeToBeaconFSM(Event);
   }
   else
   {
       ReturnEvent = RunTapeToBeaconFSM(Event);
   }

   return ReturnEvent;
}

static ES_Event_t DuringBeaconToTape(ES_Event_t Event)
{
   ES_Event_t ReturnEvent = Event;

   if ((Event.EventType == ES_ENTRY) ||
       (Event.EventType == ES_ENTRY_HISTORY))
   {
       StartBeaconToTapeFSM(Event);
   }
   else if (Event.EventType == ES_EXIT)
   {
       RunBeaconToTapeFSM(Event);
   }
   else
   {
       ReturnEvent = RunBeaconToTapeFSM(Event);
   }

   return ReturnEvent;
}

static ES_Event_t DuringTapeToDispenser(ES_Event_t Event)
{
    ES_Event_t ReturnEvent = Event;

    if ((Event.EventType == ES_ENTRY) ||
        (Event.EventType == ES_ENTRY_HISTORY))
    {
       StartTapeToDispenserFSM(Event);
    }
    else if (Event.EventType == ES_EXIT)
    {
        RunTapeToDispenserFSM(Event);
    }
    else
    {
        ReturnEvent = RunTapeToDispenserFSM(Event);
//        DB_printf("ReturnEvent: %d\n", ReturnEvent);
    }

    return ReturnEvent;
}


