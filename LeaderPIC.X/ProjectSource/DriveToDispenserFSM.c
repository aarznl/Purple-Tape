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
#include "DriveToDispenserFSM.h"
#include "TopLevelHSM.h"
#include "SPILeaderService.h"
#include "ServoService.h"

#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define ENTRY_STATE Rotate90
#define COLLECT_PERIOD 5000 // adjust later
#define FORWARD_PERIOD 2000

/*---------------------------- Module Variables ---------------------------*/
static DriveState_t CurrentState;
static uint8_t CollectCounter;
static volatile bool MovingBackwards = false;

/*---------------------------- Private Prototypes -------------------------*/

/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     RunDriveToDispenserFSM
****************************************************************************/
ES_Event_t RunDriveToDispenserFSM(ES_Event_t CurrentEvent)
{
    bool MakeTransition = false;
    DriveState_t NextState = CurrentState;
    ES_Event_t EntryEventKind = { ES_ENTRY, 0 };
    ES_Event_t ReturnEvent = CurrentEvent;

    switch (CurrentState)
    {
        /**************** ROTATE 90 away from Beacon ****************/
        case Rotate90:   
        {
            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("Rotate 90\n");
                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = TURN_CCW_90;
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
                        DB_printf("Turning 90 done\n");
                        NextState = RotateCCW;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
        }
        
        /**************** ROTATE to align with tape ****************/
        case RotateCCW:
        {
            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("Rotating\n");
                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = TURN_CCW_TAPE;
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
                    case ES_TAPE_ALIGNED:
                        DB_printf("Turning CW done\n");
                        NextState = DriveForwards10;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
        }
        
        /**************** Drive forward to pass first intersection ****************/
        case DriveForwards10:
        {
            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("DriveForwards10\n");
                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = DRIVE_FORWARD_10;
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
                        DB_printf("Drive Forward for 10cm done\n");
                        NextState = FollowTape;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
        }
        
        /**************** FOLLOW TAPE until second intersection ****************/
        case FollowTape:
        {
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
                        DB_printf("Driving FWD done\n");
                        NextState = RotateCW225;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
        }
        
        /**************** Rotate about 225 degrees to flip orientation ****************/
        case RotateCW225:
        {
            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("RotateCCW225\n");
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
                        DB_printf("Turning 225 CW done\n");
                        NextState = DriveBackwards10;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
        }
        
        /**************** Drive Backwards 10 cm to get off tape ****************/
        case DriveBackwards10:
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
                        NextState = BackwardToTurn;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
        }
        
        /**************** Drive Backwards until on last line ****************/
        case BackwardToTurn:
        {
            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("Backward\n");
                CurrentEvent.EventType = ES_LEADER_SEND;
                CurrentEvent.EventParam = BACKWARD_TAPE;
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
                    {
                            DB_printf("Backward done\n");
                            NextState = RotateCW;
                            MakeTransition = true;
                            ReturnEvent.EventType = ES_NO_EVENT;
                    }
                    break;
                }
            }
            break;
        }
        /**************** Rotate until tape aligned on the last line ****************/
        case RotateCW:
        {
            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                CollectCounter = 0;
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
                        NextState = ToDispenser;
                        MakeTransition = true;
                        ReturnEvent.EventType = ES_NO_EVENT;
                        break;
                }
            }
            break;
        }
        
        /**************** Line follow forward to the dispenser ****************/
        case ToDispenser:
        {
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
                    case SWITCH_ON:
                        DB_printf("limit switch pushed\n");
                        ES_Timer_InitTimer(SERVO_TIMER, 500);                

                        break;
                    case ES_TIMEOUT:
                        
                        if (CurrentEvent.EventParam == SERVO_TIMER) {
                            CurrentEvent.EventType = ES_LEADER_SEND;
                            CurrentEvent.EventParam = STOP;
                            PostSPILeaderService(CurrentEvent);

                            NextState = GrabBalls;
                            MakeTransition = true;
                            ReturnEvent.EventType = ES_NO_EVENT;     
                        }
                        break;
                }
            }
            break;
        }
        
        /**************** GRAB BALLS ****************/
        case GrabBalls:
        {
            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                DB_printf("Grabbing Balls\n");
                CurrentEvent.EventType = DC_CONVEYOR;
                CurrentEvent.EventParam = start;
                PostServoService(CurrentEvent);
//                
                CurrentEvent.EventType = SERVO_INTAKE;
                CurrentEvent.EventParam = start;
                PostServoService(CurrentEvent);
                
                ES_Timer_InitTimer(SERVO_TIMER, 1000);                
                
                MovingBackwards = false;

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
                    case ES_TIMEOUT:
                        if(CurrentEvent.EventParam == SERVO_TIMER)
                        {   DB_printf("Start to drive backwards");
                            CollectCounter++;
                            CurrentEvent.EventType = ES_LEADER_SEND;
                            CurrentEvent.EventParam = DRIVE_BACKWARD_20;
                            PostSPILeaderService(CurrentEvent);
                            
                            MovingBackwards = true;   // now expect CMD_DONE

                        }       
                        break;
                      
                                
                    case CMD_DONE:
                       if (MovingBackwards) {
                           if (CollectCounter == 4)
                            {
                               NextState = Done;
                              MakeTransition = true;
                              ReturnEvent.EventType = ES_NO_EVENT;
                            }
                            else
                            {
                                NextState = ToDispenser;
                                MakeTransition = true;
                                ReturnEvent.EventType = ES_NO_EVENT;
                            }                      
                       }
                        break;
                }
            }
            break;

        }
        /**************** DONE ****************/
        case Done:
        {
            if ((CurrentEvent.EventType == ES_ENTRY) ||
                (CurrentEvent.EventType == ES_ENTRY_HISTORY))
            {
                CurrentEvent.EventType = DC_CONVEYOR;
                CurrentEvent.EventParam = stop;
                PostServoService(CurrentEvent);
                
                DB_printf("In Done\n");
                CurrentEvent.EventType = COLLECT_DONE;
                PostMasterSM(CurrentEvent);   // bubble up to GameHSM
            }
            break;
        }
    }
    /*************** HANDLE TRANSITION ***************/
    if (MakeTransition == true)
    {
        CurrentEvent.EventType = ES_EXIT;
        RunDriveToDispenserFSM(CurrentEvent);

        CurrentState = NextState;

        RunDriveToDispenserFSM(EntryEventKind);
    }
//    DB_printf("ReturnEvent: %d\n", ReturnEvent);
    return ReturnEvent;
}


/****************************************************************************
 Function
     StartDriveToDispenserFSM
****************************************************************************/
void StartDriveToDispenserFSM(ES_Event_t CurrentEvent)
{
    if (CurrentEvent.EventType != ES_ENTRY_HISTORY)
    {
//        DB_printf("StartDriveToDispenserFSM called\n");
        CurrentState = ENTRY_STATE;
    }

    RunDriveToDispenserFSM(CurrentEvent);
}


/****************************************************************************
 Function
     QueryDriveToDispenserFSM
****************************************************************************/
DriveState_t QueryDriveToDispenserFSM(void)
{
    return CurrentState;
}


/***************************************************************************
 Private Functions
***************************************************************************/
