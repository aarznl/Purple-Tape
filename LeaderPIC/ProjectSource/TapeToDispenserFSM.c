/****************************************************************************
 Module
   DriveToDispenserFSM.c

 Description
   Flat FSM version of DriveToDispenserHSM implemented using
   Gen2 template transition structure. Designed to run as a
   sub-state machine inside GameHSM.

****************************************************************************/

/*----------------------------- Include Files -----------------------------*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "TapeToDispenserFSM.h"
#include "TopLevelHSM.h"
#include "SPILeaderService.h"
#include "ServoService.h"

#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define ENTRY_STATE FollowTape_Back
#define COLLECT_PERIOD 5000 // adjust later
#define FORWARD_PERIOD 2000

/*---------------------------- Module Variables ---------------------------*/
static BackState_t CurrentState;
static uint8_t CollectCounter;


/*---------------------------- Private Prototypes -------------------------*/

/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     RunDriveToDispenserFSM
****************************************************************************/
ES_Event_t RunTapeToDispenserFSM(ES_Event_t CurrentEvent)
{
    bool MakeTransition = false;
    BackState_t NextState = CurrentState;
    ES_Event_t EntryEventKind = { ES_ENTRY, 0 };
    ES_Event_t ReturnEvent = CurrentEvent;

    switch (CurrentState)
    {
        
        /**************** FOLLOW ****************/
        case Forward_Back:

            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("Following Tape\n");
                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = FORWARD_TAPE;
                PostSPILeaderService(CurrentEvent);
                
            }
            else if (CurrentEvent.EventType == ES_EXIT)
            {
                // stop tape motors if needed
            }
            else
            {
                switch (CurrentEvent.EventType)
                {
                    case ES_BACK_RIGHT_DETECT:
                        DB_printf("Tape detected, ready to turn\n");
                        NextState = RotateCW90_Back;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;

        case RotateCW90_Back:                

            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {

                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = TURN_CW_90;
                PostSPILeaderService(CurrentEvent);

            }
            else if (CurrentEvent.EventType == ES_EXIT)
            {
                // stop rotation motors here if needed
            }
            else
            {
                switch (CurrentEvent.EventType)
                {
                    case CMD_DONE:
                        DB_printf("Turning 90 CW done\n");
                        NextState = FollowTape_Back;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
            
        case FollowTape_Back:

            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("Following Tape\n");
                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = FORWARD_TAPE;
                PostSPILeaderService(CurrentEvent);
                
            }
            else if (CurrentEvent.EventType == ES_EXIT)
            {
                // stop tape motors if needed
            }
            else
            {
                switch (CurrentEvent.EventType)
                {
                    case ES_BACK_RIGHT_DETECT:
                        NextState = RotateCW225_Back;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
        case RotateCW225_Back:

            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("RotateCCW90\n");
                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = TURN_CW_225;
                PostSPILeaderService(CurrentEvent);  
            }
            else if (CurrentEvent.EventType == ES_EXIT)
            {
                // stop tape motors if needed
            }
            else
            {
                switch (CurrentEvent.EventType)
                {
                    case CMD_DONE:
                        DB_printf("Turning 90 CCW done\n");
                        NextState = BackwardToTurn_Back;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
            
        case DriveBackwards10_Back:
        {
            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("DriveBackwards10\n");
                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = DRIVE_BACKWARD_10;
                PostSPILeaderService(CurrentEvent);  
            }
            else if (CurrentEvent.EventType == ES_EXIT)
            {
                // stop tape motors if needed
            }
            else
            {
                switch (CurrentEvent.EventType)
                {
                    case CMD_DONE:
                        DB_printf("Drive Backward for 10cm done\n");
                        NextState = BackwardToTurn_Back;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
        }
            
        case BackwardToTurn_Back:

            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("Forward\n");
                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = BACKWARD_TAPE;
                PostSPILeaderService(CurrentEvent);
//                ES_Timer_InitTimer(FORWARD_TIMER, FORWARD_PERIOD);
            }
            else if (CurrentEvent.EventType == ES_EXIT)
            {
                // stop tape motors if needed
            }
            else
            {
                switch (CurrentEvent.EventType)
                {
                    case ES_BACK_RIGHT_DETECT:
                        DB_printf("Backward done\n");
                        NextState = RotateCW_Back;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
            
        case RotateCW_Back:

            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("RotateCW\n");
                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = TURN_CW_TAPE;
                PostSPILeaderService(CurrentEvent);
            }
            else if (CurrentEvent.EventType == ES_EXIT)
            {
                // stop tape motors if needed
            }
            else
            {
                switch (CurrentEvent.EventType)
                {
                    case ES_TAPE_ALIGNED:
                        DB_printf("Turn CW done\n");
                        NextState = ToDispenser_Back;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
            
        case ToDispenser_Back:

            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("ToDispenser\n");
                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = DRIVE_FORWARD;
                PostSPILeaderService(CurrentEvent);
            }
            else if (CurrentEvent.EventType == ES_EXIT)
            {
                // stop tape motors if needed
            }
            else
            {
                switch (CurrentEvent.EventType)
                {
                    case START:
                        DB_printf("limit switch pushed\n");
                        CurrentEvent.EventType = ES_LEADER_SEND;
                        CurrentEvent.EventParam = STOP;
                        PostSPILeaderService(CurrentEvent);
                
                        NextState = GrabBalls_Back;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
        /**************** GRAB BALLS ****************/
        case GrabBalls_Back:

            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("Grabbing Balls\n");
//                CurrentEvent.EventType = DC_CONVEYOR;
//                CurrentEvent.EventParam = start;
//                PostServoService(CurrentEvent);
//                
                CurrentEvent.EventType = SERVO_INTAKE;
                CurrentEvent.EventParam = start;
                PostServoService(CurrentEvent);
                
                ES_Timer_InitTimer(SERVO_TIMER, 500);
                
                CollectCounter = 0;

            }
            else if (CurrentEvent.EventType == ES_EXIT)
            {              
                CurrentEvent.EventType = SERVO_INTAKE;
                CurrentEvent.EventParam = stop;
                PostServoService(CurrentEvent);
            }
            else
            {
                switch (CurrentEvent.EventType)
                {
                    case CMD_DONE:
                        if (CollectCounter == 9)
                        {
                            NextState = Done_Back;
                            MakeTransition = true;
                            ReturnEvent.EventType = ES_NO_EVENT;
                        }
                        
                        else
                        {
                            NextState = ToDispenser_Back;
                            MakeTransition = true;
                            ReturnEvent.EventType = ES_NO_EVENT;
                        }                      
                        
                        break;

                    case ES_TIMEOUT:
                        if(CurrentEvent.EventParam == SERVO_TIMER)
                        {
                            CollectCounter++;
                            CurrentEvent.EventType = ES_LEADER_SEND;
                            CurrentEvent.EventParam = DRIVE_BACKWARD_10;
                            PostSPILeaderService(CurrentEvent);
                        }
                        
                        break;
                }
            }
            break;


        /**************** DONE ****************/
        case Done_Back:

            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("In Done\n");
                CurrentEvent.EventType = COLLECT_DONE;
                PostMasterSM(CurrentEvent);   // bubble up to GameHSM
            }
            break;
    }

    /*************** HANDLE TRANSITION ***************/
    if (MakeTransition == true)
    {
        CurrentEvent.EventType = ES_EXIT;
        RunTapeToDispenserFSM(CurrentEvent);

        CurrentState = NextState;

        RunTapeToDispenserFSM(EntryEventKind);
    }
//    DB_printf("ReturnEvent: %d\n", ReturnEvent);
    return ReturnEvent;
}


/****************************************************************************
 Function
     StartDriveToDispenserFSM
****************************************************************************/
void StartTapeToDispenserFSM(ES_Event_t CurrentEvent)
{
    if (CurrentEvent.EventType != ES_ENTRY_HISTORY)
    {
//        DB_printf("StartDriveToDispenserFSM called\n");
        CurrentState = ENTRY_STATE;
    }

    RunTapeToDispenserFSM(CurrentEvent);
}


/****************************************************************************
 Function
     QueryDriveToDispenserFSM
****************************************************************************/
BackState_t QueryTapeToDispenserFSM(void)
{
    return CurrentState;
}


/***************************************************************************
 Private Functions
***************************************************************************/
