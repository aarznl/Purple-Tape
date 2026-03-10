
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "PIC32_SPI_HAL.h"
#include "SPIFollowerService.h"
#include "MotorSM.h"
#include "BeaconService.h"
#include "dbprintf.h"

#include <xc.h>
#include <sys/attribs.h>

static uint8_t MyPriority;
static uint8_t NewReceived = 0;
volatile static uint8_t LastReceived;

static volatile uint8_t SPI_Status = FOLLOWER_DONE;

static volatile uint8_t Delayed_Status = FOLLOWER_DONE;


bool InitSPIFollowerService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
  __builtin_disable_interrupts();
  // SPI SETUP FOR FOLLOWER MODE
  SPISetup_BasicConfig(SPI_SPI1);
  SPISetup_DisableSPI(SPI_SPI1);
  SPISetup_SetFollower(SPI_SPI1);
  SPISetup_MapSSInput(SPI_SPI1, SSIN_PIN);
  SPISetup_MapSDOutput(SPI_SPI1, SDO_PIN);
  SPISetup_MapSDInput(SPI_SPI1, SDI_PIN);
  SPISetup_SetClockIdleState(SPI_SPI1, SPI_CLK_HI);
  SPISetup_SetActiveEdge(SPI_SPI1, SPI_SECOND_EDGE);
  SPISetup_SetXferWidth(SPI_SPI1, SPI_8BIT);
  SPISetEnhancedBuffer(SPI_SPI1, false); // should I use this or no??
  SPI1BUF = 0x00;   // preload first byte
  
  SPI1STATCLR = _SPI1STAT_SPIROV_MASK;

  volatile uint8_t dummy;
  dummy = SPI1BUF;
  dummy = SPI1BUF;

  SPI1BUF = 0xF1;   // preload READY byte
  
  SPISetup_EnableSPI(SPI_SPI1);
  
  IFS1CLR = _IFS1_SPI1RXIF_MASK;   // Clear RX flag
  IFS1CLR = _IFS1_SPI1EIF_MASK;    // Clear error flag

  IPC7bits.SPI1IP = 7;   // Priority level 7
  IPC7bits.SPI1IS = 0;   // Subpriority

  IEC1SET = _IEC1_SPI1RXIE_MASK;   // Enable SPI1 RX interrupt
  
  
  __builtin_enable_interrupts();  
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool PostSPIFollowerService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunSPIFollowerService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
  switch (ThisEvent.EventType)
  {
      case ES_INIT:
          DB_printf("\rES_INIT received in Follower Service %d\r\n", MyPriority);
          break;
      case ES_FROM_LEADER:
      {
          ES_Event_t EventSend;
          switch (ThisEvent.EventParam) {
              
              case DRIVE_FORWARD:
              {
                  EventSend.EventType = ES_DRIVEFORWARD;
                  PostMotorSM(EventSend);
                  break;
              }
              
              case DRIVE_FORWARD_5:
              {
                  EventSend.EventType = ES_DRIVEFORWARD_5;
                  PostMotorSM(EventSend);
                  break;
              }
              
              case DRIVE_FORWARD_10:
              {
                  EventSend.EventType = ES_DRIVEFORWARD_10;
                  PostMotorSM(EventSend);
                  break;
              }  
              
              case DRIVE_BACKWARD:
              {
                  EventSend.EventType = ES_DRIVEBACKWARD;
                  PostMotorSM(EventSend);
                  break;
              } 
              
              case DRIVE_BACKWARD_5:
              {
                  EventSend.EventType = ES_DRIVEBACKWARD_5;
                  PostMotorSM(EventSend);
                  break;
              } 
             
              case DRIVE_BACKWARD_10:
              {
                  EventSend.EventType = ES_DRIVEBACKWARD_10;
                  PostMotorSM(EventSend);
                  break;
              }  
              
              case DRIVE_BACKWARD_20:
              {
                  EventSend.EventType = ES_DRIVEBACKWARD_20;
                  PostMotorSM(EventSend);
                  break;
              }
              
              case DRIVE_BACKWARD_30:
              {
                  EventSend.EventType = ES_DRIVEBACKWARD_30;
                  PostMotorSM(EventSend);
                  break;
              }
              
              case TURN_90_CW:
              {
                  EventSend.EventType = ES_TURN90CW;
                  PostMotorSM(EventSend);                  
                  break;
              }              

              case TURN_90_CCW:
              {
                  EventSend.EventType = ES_TURN90CCW;
                  PostMotorSM(EventSend);    
                  break;
              }
              
              case TURN_CW:
              {
                  EventSend.EventType = ES_TURNCW;
                  PostMotorSM(EventSend);   
                  PostBeaconService(EventSend);                       
                  break;
              }
              
              case TURN_CCW:
              {
                  EventSend.EventType = ES_TURNCCW;
                  PostMotorSM(EventSend);                       
                  break;
              }
              
              case TURN_CCW_TAPE:
              {
                  EventSend.EventType = ES_CCW_TAPE;
                  PostMotorSM(EventSend);                       
                  break;
              }
              
              case TURN_CW_TAPE:
              {
                  EventSend.EventType = ES_CW_TAPE;
                  PostMotorSM(EventSend);                       
                  break;
              }
              
              case FORWARD_TAPE:
              {
                  EventSend.EventType = ES_FWD_TAPE;
                  PostMotorSM(EventSend);                       
                  break;
              }
              
              case BACKWARD_TAPE:
              {
                  EventSend.EventType = ES_BWD_TAPE;
                  PostMotorSM(EventSend);                       
                  break;
              }
              
              case TURN_45_CW:
              {
                  EventSend.EventType = ES_TURN45CW;
                  PostMotorSM(EventSend);                       
                  break;
              }
              
              case TURN_180_CW:
              {
                  EventSend.EventType = ES_TURN180CW;
                  PostMotorSM(EventSend);                       
                  break;
              }
              
              case TURN_225_CW:
              {
                  EventSend.EventType = ES_TURN225CW;
                  PostMotorSM(EventSend);                       
                  break;
              }
              
              case BEACON_TURN_CW:
              {
                  EventSend.EventType = ES_LOOK_FOR_BEACON;
                  PostBeaconService(EventSend);
                  EventSend.EventType = ES_TURNCW;
                  PostMotorSM(EventSend);       
                   
                  break;
              }

              case BEACON_TURN_CCW:
              {
                  EventSend.EventType = ES_TURNCCW;
                  PostMotorSM(EventSend);  
                  EventSend.EventType = ES_LOOK_FOR_BEACON;
                  PostBeaconService(EventSend);   
                  break;
              }
              
              case ROBOT_STOP:
              {
                   DB_printf("Stop Post\n");

                  EventSend.EventType = ES_STOP;
                  PostMotorSM(EventSend);                       
                  break;
              }

              case QUERY:
              {
                  break;
              }
          }
          //DB_printf("leader command received: %u\n",ThisEvent.EventParam);
          break;
      }
      
//      case ES_STOP:
//      {
//          DB_printf("robot stops\n");
//          UpdateStatus(ROBOT_STOP);
////          SPI_Status = ROBOT_STOP;
//          break;
//      }
      
      case ES_TIMEOUT:
      {
          if (ThisEvent.EventParam == DOUBLE_DELAY) {
              UpdateStatus(Delayed_Status);
          }
      }
  }
  return ReturnEvent;
}

void UpdateStatus(uint8_t Status)
{
    if (SPI_Status != Status) {
        SPI_Status = Status;
    } else { 
        SPI_Status = NEW_C0MMAND_INCOMING;
        Delayed_Status = Status;
        ES_Timer_InitTimer(DOUBLE_DELAY, 100);
    }
} 

/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

void __ISR(_SPI1_VECTOR, IPL7SOFT) SPI1_Handler(void)
{
    if (IFS1bits.SPI1RXIF)
    {
        if (SPI1STATbits.SPIROV) // clear overflow if necessary
        {
            SPI1STATCLR = _SPI1STAT_SPIROV_MASK;
        }
        uint8_t NewReceived = (uint8_t)SPI1BUF;

        IFS1CLR = _IFS1_SPI1RXIF_MASK;
        

//        if (NewReceived != 0xAA){SPI_Status = FOLLOWER_BUSY;}
        
        SPI1BUF = SPI_Status;          // always shift out latest status

        if (NewReceived != LastReceived) // only post an event if it's somethign diff than last time
        {
            ES_Event_t ThisEvent;
            ThisEvent.EventType = ES_FROM_LEADER;
            ThisEvent.EventParam = NewReceived;
            PostSPIFollowerService(ThisEvent);
            //PostMotorSM(ThisEvent);
        }

        if (QueryMotorSM() == FollowerReady) // if ready, change status to ready again
        {
            SPI_Status = FOLLOWER_DONE;
        }
        LastReceived = NewReceived;
    }
}
