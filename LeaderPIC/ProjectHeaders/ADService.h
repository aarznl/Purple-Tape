/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ADService_H
#define ADService_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitADService(uint8_t Priority);
bool PostADService(ES_Event_t ThisEvent);
ES_Event_t RunADService(ES_Event_t ThisEvent);
uint16_t GetTimeInterval(void);
static void ChangeSpeedVal(void);

#endif /* ADService_H */

