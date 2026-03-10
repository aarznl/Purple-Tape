
#ifndef LineFollower_H
#define LineFollower_H

typedef enum {
    Look4Tape = 0,     
    FollowingTape,
} FollowState_t;

bool InitLineFollower(uint8_t Priority);
bool PostLineFollower(ES_Event_t ThisEvent);
ES_Event_t RunLineFollower(ES_Event_t ThisEvent);

void QueryLineVals(uint32_t *buffer);

#endif /* BeaconService_H */