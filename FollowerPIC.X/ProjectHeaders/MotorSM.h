/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef MotorSM_H
#define MotorSM_H

#define FOLLOWER_BUSY  0xF0
#define FOLLOWER_DONE  0xF1

#define TAPE_DETECTED 0xEA
#define TURN_DONE 0xEC
#define TURN_DONE_180 0xDF
#define FWD_BWD_DONE 0xDE

#define LEFT 0
#define RIGHT 1

//extern volatile uint8_t SPI_Status;

#include "ES_Types.h"

typedef enum
{
    Forward, Backward, Stopped
}MotorDirection_t;

typedef enum
{
    InitMotorSMState,FollowerBusy,FollowerReady
}MotorSMState_t;

typedef enum {
    TurnAwayFromBeacon = 0,
    AlignFromBeacon,     
    ForwardToDispenser,
    AwayFromDispenser,
    RotateToDispenser,
    DispenserToBucket,
    looking4TapeFromBeacon,
    looking4TapeTurning,
} FollowState_t;

// Public Function Prototypes

bool InitMotorSM(uint8_t Priority);
bool PostMotorSM(ES_Event_t ThisEvent);
ES_Event_t RunMotorSM(ES_Event_t ThisEvent);
MotorSMState_t QueryMotorSM(void);
//void UpdateRPM(float RPMValue, bool whichMotor);
void Control (int16_t CurrentLeftRPM, int16_t CurrentRightRPM);
bool Check4BackRightLine(void);
bool Check4CenterMiddleLine(void);

#endif /* MotorSM_H */

