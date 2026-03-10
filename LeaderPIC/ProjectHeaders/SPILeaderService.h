
#ifndef SPILeaderService_H
#define SPILeaderService_H


#include "ES_Types.h"
#include "ES_Configure.h"

#define SS_PIN SPI_RPA0
#define SDO_PIN SPI_RPA1
#define SDI_PIN SPI_RPB5

#define FOLLOWER_BUSY  0xF0
#define FOLLOWER_DONE  0xF1

#define TURN_CW 0x71
#define TURN_CCW 0x72
#define TURN_CW_45 0x73
#define BACKWARD 0x68
#define TURN_CCW_90 0x70
#define DRIVE_FORWARD 0x67
#define DRIVE_BACKWARD 0x68
#define TURN_CW_180 0x74
#define STOP 0xEB
#define TURN_CW_90 0x69
#define TURN_CW_225 0x76
#define DRIVE_BACKWARD_10 0x77
#define DRIVE_FORWARD_10 0x78
#define DRIVE_BACKWARD_5 0x79
#define DRIVE_FORWARD_5 0x80
#define DRIVE_BACKWARD_20 0xEA
#define DRIVE_BACKWARD_30 0x50

#define CMD_DONE 0xEC

#define ROBOT_STOP 0xEB

#define FORWARD_TAPE 0x51
#define BACK_RIGHT_DETECT 0x52
#define TAPE_ALIGNED 0x53
#define TURN_CCW_TAPE 0x54
#define BACKWARD_TAPE 0x55
#define TURN_CW_TAPE 0x56

#define BEACON_TURN_CW 0x57
#define BEACON_TURN_CCW 0x58

#define DRIVE_FORWARD_TAPE 0x59


#define QUERY 0xAA

#define NEW_C0MMAND_INCOMING 0xFF

//#define BEACON_G_DETECT 0xB0
//#define BEACON_B_DETECT 0xB1
#define BEACON_L_DETECT 0xB2
#define BEACON_R_DETECT 0xB3

bool InitSPILeaderService(uint8_t Priority);
bool PostSPILeaderService(ES_Event_t ThisEvent);
ES_Event_t RunSPILeaderService(ES_Event_t ThisEvent);

#endif /* SPILeaderService_H */