/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef MotorService_H
#define MotorService_H

#include "ES_Types.h"

// Public Function Prototypes

typedef enum
{
    InitMotorState,
            MotorWaiting4Command,
            MotorRunningCommand,
            Looking4Beacon,
            TapeDetected
} MotorServiceState_t ;

bool InitMotorService(uint8_t Priority);
bool PostMotorService(ES_Event_t ThisEvent);
ES_Event_t RunMotorService(ES_Event_t ThisEvent);
MotorServiceState_t QueryMotorService(void);
static void changeDirection(bool whichDirection, bool whichMotor);
void UpdateRPM(float RPMValue, bool whichMotor);
void ExecuteCommand (uint8_t cmd);

#endif /* ServTemplate_H */

