/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ServOrient_H
#define ServOrient_H

#include "ES_Types.h"

#define ORIENTATION_DONE_G 0xEF
#define ORIENTATION_DONE_B 0xEE
// Public Function Prototypes

bool InitOrientService(uint8_t Priority);
bool PostOrientService(ES_Event_t ThisEvent);
ES_Event_t RunOrientService(ES_Event_t ThisEvent);

#endif /* ServTemplate_H */

