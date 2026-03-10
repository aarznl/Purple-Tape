/****************************************************************************
 Module
   DriveToDispenserFSM.h

 Description
   Header file for the flat FSM controlling the drive to the dispenser.
   Defines states, events, and interface functions.

****************************************************************************/

#ifndef DRIVE_TO_DISPENSER_FSM_H
#define DRIVE_TO_DISPENSER_FSM_H

#include "ES_Configure.h"
#include "ES_Framework.h"

/*----------------------------- State Definitions -------------------------*/
typedef enum {
    Rotate90,
    RotateCCW,
    DriveForwards10,
    FollowTape,
    RotateCW225,
    DriveBackwards10,
    BackwardToTurn,
    RotateCW,
    ToDispenser,
    GrabBalls,
    Done
} DriveState_t;

/*----------------------------- Public Functions --------------------------*/

ES_Event_t RunDriveToDispenserFSM(ES_Event_t ThisEvent);
DriveState_t QueryDriveToDispenserFSM(void);
void StartDriveToDispenserFSM ( ES_Event_t CurrentEvent );

#endif // DRIVE_TO_DISPENSER_FSM_H