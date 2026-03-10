/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ServTemplate_H
#define ServTemplate_H

#include "ES_Types.h"

// Define servo event parameters
typedef enum {
    Green = 0,     
    Blue, 
} ServoEventParam_t;

typedef enum {
    start = 0,     
    stop, 
} DCEventParam_t;

// Public Function Prototypes

bool InitServoService(uint8_t Priority);
bool PostServoService(ES_Event_t ThisEvent);
ES_Event_t RunServoService(ES_Event_t ThisEvent);

#endif /* ServTemplate_H */

