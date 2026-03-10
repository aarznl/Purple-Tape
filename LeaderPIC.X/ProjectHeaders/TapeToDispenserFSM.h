/****************************************************************************
 Module
   DriveToDispenserFSM.h

 Description
   Header file for the flat FSM controlling the drive to the dispenser.
   Defines states, events, and interface functions.

****************************************************************************/

#ifndef TAPE_TO_DISPENSER_FSM_H
#define TAPE_TO_DISPENSER_FSM_H

#include "ES_Configure.h"
#include "ES_Framework.h"

/*----------------------------- State Definitions -------------------------*/
typedef enum {
    Forward_Back,
    RotateCW90_Back,
    FollowTape_Back,
    RotateCW225_Back,
    DriveBackwards10_Back,
    BackwardToTurn_Back,
    RotateCW_Back,
    ToDispenser_Back,
    GrabBalls_Back,
    Done_Back
} BackState_t;

/*----------------------------- Public Functions --------------------------*/

ES_Event_t RunTapeToDispenserFSM(ES_Event_t ThisEvent);
BackState_t QueryTapeToDispenserFSM(void);
void StartTapeToDispenserFSM ( ES_Event_t CurrentEvent );

#endif // DRIVE_TO_DISPENSER_FSM_H