
#ifndef BeaconService_H
#define BeaconService_H

#include "ES_Types.h"


#define BEACON_G_PERIOD_US 300
#define BEACON_B_PERIOD_US 700
#define BEACON_R_PERIOD_US 1100
#define BEACON_L_PERIOD_US 500

typedef enum {
    BEACON_NONE = 0,
    BEACON_G,
    BEACON_B,
    BEACON_R,
    BEACON_L
} BeaconType_t;

#define DRIVING_TO_BEACON_L 0xED

#define BEACON_L_DETECT 0xB2
#define BEACON_R_DETECT 0xB3

typedef enum {
    InitBeaconState,
    Looking,
    NotLooking,
}BeaconState_t;

bool InitBeaconService(uint8_t Priority);
bool PostBeaconService(ES_Event_t ThisEvent);
ES_Event_t RunBeaconService(ES_Event_t ThisEvent);

#endif /* BeaconService_H */