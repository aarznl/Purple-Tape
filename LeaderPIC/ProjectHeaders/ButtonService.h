/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ButtonService_H
#define ButtonService_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitButtonService(uint8_t Priority);
bool PostButtonService(ES_Event_t ThisEvent);
ES_Event_t RunButtonService(ES_Event_t ThisEvent);
bool Check4ButtonDown(void);

#endif /* ADService_H */

