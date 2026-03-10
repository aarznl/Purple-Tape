
#ifndef SPIFollowerService_H
#define SPIFollowerService_H

#include "ES_Types.h"
#include "ES_Configure.h"

#define SSIN_PIN SPI_RPA0
#define SDO_PIN SPI_RPA1
#define SDI_PIN SPI_RPB5

#define FOLLOWER_BUSY  0xF0
#define FOLLOWER_DONE  0xF1

#define TAPE_DETECTED 0xEA
#define TAPE_DETECTED_1 0x50

#define CMD_DONE 0xEC

#define ROBOT_STOP 0xEB

#define QUERY 0xAA

#define DRIVE_FORWARD 0x67
#define DRIVE_BACKWARD 0x68
#define TURN_90_CW 0x69
#define TURN_90_CCW 0x70
#define TURN_CW 0x71
#define TURN_CCW 0x72
#define TURN_45_CW 0x73
#define TURN_180_CW 0x74
#define TURN_225_CW 0x76
#define DRIVE_BACKWARD_10 0x77
#define DRIVE_FORWARD_10 0x78
#define DRIVE_BACKWARD_5 0x79
#define DRIVE_FORWARD_5 0x80
#define DRIVE_BACKWARD_20 0xEA
#define DRIVE_BACKWARD_30 0x50

#define FORWARD_TAPE 0x51
#define BACK_RIGHT_DETECT 0x52
#define TAPE_ALIGNED 0x53
#define TURN_CCW_TAPE 0x54
#define BACKWARD_TAPE 0x55
#define TURN_CW_TAPE 0x56

#define BEACON_TURN_CW 0x57
#define BEACON_TURN_CCW 0x58



#define NEW_C0MMAND_INCOMING 0xFF


bool InitSPIFollowerService(uint8_t Priority);
bool PostSPIFollowerService(ES_Event_t ThisEvent);
ES_Event_t RunSPIFollowerService(ES_Event_t ThisEvent);

void UpdateStatus(uint8_t Status);

#endif /* SPIFollowerService_H */