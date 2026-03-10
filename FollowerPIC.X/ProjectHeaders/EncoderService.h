/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef EncoderService_H
#define EncoderServicee_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitEncoderService(uint8_t Priority);
bool PostEncoderService(ES_Event_t ThisEvent);
ES_Event_t RunEncoderService(ES_Event_t ThisEvent);
void DriveDistance (int16_t Distance);


#endif /* ServTemplate_H */

