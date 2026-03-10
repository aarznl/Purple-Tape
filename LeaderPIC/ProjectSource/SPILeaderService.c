
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "PIC32_SPI_HAL.h"
#include "SPILeaderService.h"
#include "dbprintf.h"
#include "TopLevelHSM.h"
#include "OrientService.h"

#include <xc.h>
#include <sys/attribs.h>


#define QUERY_PERIOD 100

static uint8_t MyPriority;
static uint8_t NewReceived = 0;
volatile static uint8_t LastReceived;

bool InitSPILeaderService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
  __builtin_disable_interrupts();
  // SPI SETUP FOR LEADER MODE
//  InitSPI();
  SPISetup_BasicConfig(SPI_SPI1);
  SPISetup_DisableSPI(SPI_SPI1);
  
  // Make sure SS idles HIGH before SPI re-enable
  SPI1CONCLR = _SPI1CON_ON_MASK;   // ensure SPI fully OFF
    
  SPISetup_SetLeader(SPI_SPI1, SPI_SMP_MID);
  SPISetup_SetBitTime(SPI_SPI1, 10000);
  SPISetup_MapSSOutput(SPI_SPI1, SS_PIN);
  SPISetup_MapSDOutput(SPI_SPI1, SDO_PIN);
  SPISetup_MapSDInput(SPI_SPI1, SDI_PIN);
  SPISetup_SetClockIdleState(SPI_SPI1, SPI_CLK_HI);
  SPISetup_SetActiveEdge(SPI_SPI1, SPI_SECOND_EDGE);
  SPISetup_SetXferWidth(SPI_SPI1, SPI_8BIT);
  SPISetEnhancedBuffer(SPI_SPI1, false); // should I use this or no??
  SPISetup_EnableSPI(SPI_SPI1);
  
  for(int i = 0; i < 5; i++){SPIOperate_SPI1_Send8(0xFF);} //dummy bytes for startup sync
  
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

bool PostSPILeaderService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunSPILeaderService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
  switch (ThisEvent.EventType)
  {
      case ES_INIT:
          ES_Timer_InitTimer(SPI_TIMER, 500);
          break;
      
      case ES_LEADER_SEND:
      {
          uint8_t NewCommand = ThisEvent.EventParam;
          for (int i = 1; i<25; i++){}
          SPIOperate_SPI1_Send8(NewCommand);
          break;
      }
      
      case ES_TIMEOUT:
      {
          if (ThisEvent.EventParam == SPI_TIMER) {
    //          DB_printf("Querying the follower pic...\n");
              SPIOperate_SPI1_Send8(QUERY);
              ES_Timer_InitTimer(SPI_TIMER, QUERY_PERIOD);
          }
          break;
      }
      // Key press testing
      case ES_NEW_KEY:
        {
            switch (ThisEvent.EventParam)
            {
                case 'a':
                {
                    DB_printf("Pressed A for forward\n");
                    SPIOperate_SPI1_Send8Wait(0x06);
                    break;
                }
            }
            break;
      }
      case ES_FROM_FOLLOWER:
      {
//          DB_printf("received byte: %u\n",ThisEvent.EventParam);
          switch (ThisEvent.EventParam)
          {
              case FOLLOWER_BUSY:
              {
//                  DB_printf("busy... ");
                  break;
              }
              case FOLLOWER_DONE:
              {
//                  DB_printf("ready... ");
                
                break;
              }
              case BEACON_L_DETECT:
              {
                DB_printf("beacon L detected\n");
                ThisEvent.EventType =  ES_BEACON_DETECT;
                ThisEvent.EventParam = ORIENTATION_DONE_G;
//                PostOrientService(ThisEvent);
                PostMasterSM(ThisEvent); // needed for tape to beacon
                break;
              }
              case BEACON_R_DETECT:
              {
                DB_printf("beacon R detected\n");
                ThisEvent.EventType =  ES_BEACON_DETECT;
                ThisEvent.EventParam = ORIENTATION_DONE_B;
//                PostOrientService(ThisEvent);
                PostMasterSM(ThisEvent); // needed for tape to beacon
                break;
              }
              case CMD_DONE:
              {
                ThisEvent.EventType =  CMD_DONE;
                PostMasterSM(ThisEvent);
                break;
              }
              case BACK_RIGHT_DETECT:
              {
                DB_printf("Back right detected sent to leader\n");
                ThisEvent.EventType = ES_BACK_RIGHT_DETECT,
                PostMasterSM(ThisEvent);
                break;
              }
              case TAPE_ALIGNED:
              {
                DB_printf("Tape aligned detected sent to leader\n");
                ThisEvent.EventType = ES_TAPE_ALIGNED,
                PostMasterSM(ThisEvent);
                break;
              }
              // tape detect
              // beacon detect
              default:
//                  DB_printf("unrecognized byte: %u\n",ThisEvent.EventParam);
                  break;
          }
//          DB_printf("New Command Received from follower: %u\n",ThisEvent.EventParam);
          break;
      }
      
  }
  return ReturnEvent;
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
        if (SPI1STATbits.SPIROV) //clear overflow
        {
            SPI1STATCLR = _SPI1STAT_SPIROV_MASK;
        }

        uint8_t NewReceived = (uint8_t)SPI1BUF;
        IFS1CLR = _IFS1_SPI1RXIF_MASK;

//        DB_printf("RX: 0x%02X\n", NewReceived);
//        if (NewReceived != LastReceived) // only post an event if it's somethign diff than last time
//        DB_printf("here!, %u",NewReceived );
        
        if (NewReceived != LastReceived)
        {
            ES_Event_t ThisEvent;
            ThisEvent.EventType = ES_FROM_FOLLOWER;
            ThisEvent.EventParam = NewReceived;
            PostSPILeaderService(ThisEvent);            
            switch (ThisEvent.EventParam)
            {
                case FOLLOWER_BUSY:
//                    DB_printf("busy...\n");
                    break;
                case FOLLOWER_DONE:
//                    DB_printf("ready!\n");
                    break;

            }
//            DB_printf("\n");
        }
        LastReceived = NewReceived;
    }
}
