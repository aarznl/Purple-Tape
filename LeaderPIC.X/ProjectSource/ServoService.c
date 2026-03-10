/****************************************************************************
 Module
   ServoService.c

 Description
   Service to control servo motors using the PWM HAL on PIC32.
   Receives events to move servo(s) to a specific position (duty cycle or pulse width).

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ServoService.h"
#include "PWM_PIC32.h"
#include "TopLevelHSM.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define SERVO_CHANNEL_1 1
#define SERVO_CHANNEL_2 2
#define SERVO_CHANNEL_3 3
#define SERVO_CHANNEL_4 4
#define SERVO_CHANNEL_5 5

#define SERVO_DEFAULT_DUTY 10  // default position
#define INTAKE_DEFAULT_DUTY 7  // default position
#define INDICATOR_DEFAULT_DUTY 8  // default position
#define ARM_DEFAULT_DUTY 15

#define DC_CONVEYOR_OUTPUT LATBbits.LATB12
//#define DC_WHEEL_OUTPUT PORTBbits.RB13

/*---------------------------- Module Functions ---------------------------*/
// prototypes for private functions
static void SetServoDuty(uint8_t channel, uint8_t duty);
static void SetServoPulseWidth(uint8_t channel, uint16_t pulseWidth);

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
bool InitServoService(uint8_t Priority)
{
    ES_Event_t ThisEvent;
    MyPriority = Priority;

    /********************************************
     Initialize PWM system for servo control
    *********************************************/
    PWMSetup_BasicConfig(5);                // use 2 channels
    PWMSetup_AssignChannelToTimer(SERVO_CHANNEL_1, _Timer3_);
    PWMSetup_AssignChannelToTimer(SERVO_CHANNEL_2, _Timer3_);
    PWMSetup_AssignChannelToTimer(SERVO_CHANNEL_3, _Timer3_);
    PWMSetup_AssignChannelToTimer(SERVO_CHANNEL_4, _Timer3_);
    // PWMSetup_AssignChannelToTimer(SERVO_CHANNEL_5, _Timer3_);
    PWMSetup_MapChannelToOutputPin(SERVO_CHANNEL_1, PWM_RPB4); //arm
    PWMSetup_MapChannelToOutputPin(SERVO_CHANNEL_2, PWM_RPB11); //intake
    PWMSetup_MapChannelToOutputPin(SERVO_CHANNEL_3, PWM_RPA3); //indicator
    PWMSetup_MapChannelToOutputPin(SERVO_CHANNEL_4, PWM_RPB13); //bucket
 //   PWMSetup_MapChannelToOutputPin(SERVO_CHANNEL_5, PWM_RPB2); //arm

    // set default position
   PWMOperate_SetDutyOnChannel(SERVO_DEFAULT_DUTY, SERVO_CHANNEL_1);
    PWMOperate_SetDutyOnChannel(INTAKE_DEFAULT_DUTY, SERVO_CHANNEL_2);
    PWMOperate_SetDutyOnChannel(INDICATOR_DEFAULT_DUTY, SERVO_CHANNEL_3);
    PWMOperate_SetDutyOnChannel(SERVO_DEFAULT_DUTY, SERVO_CHANNEL_4);
//    PWMOperate_SetDutyOnChannel(ARM_DEFAULT_DUTY, SERVO_CHANNEL_5);
    

    // set the DC motor for conveyor
    TRISBbits.TRISB12 = 0; // output
    DC_CONVEYOR_OUTPUT = 0;
//    TRISBbits.TRISB13 = 0;
//    DC_WHEEL_OUTPUT = 0;
    

    // post ES_INIT event
    ThisEvent.EventType = ES_INIT;
    return ES_PostToService(MyPriority, ThisEvent);
}

bool PostServoService(ES_Event_t ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunServoService(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT;

    ES_Event_t NewEvent;
   
    switch (ThisEvent.EventType)
    {
        case SERVO_INDICATOR:
            DB_printf("In servo indicator\n");
            if(ThisEvent.EventParam == Blue){
                PWMOperate_SetDutyOnChannel(13,3);
            }else{
                PWMOperate_SetDutyOnChannel(4,3);
            }
            break;

        case SERVO_INTAKE:
//            DB_printf("In servo intake\n");
            switch(ThisEvent.EventParam)
            {
                case start:
                    PWMOperate_SetDutyOnChannel(11,2);
                    break;
                
                case stop:
                    PWMOperate_SetDutyOnChannel(7,2);
                    break;
            }
            
            break;

        case SERVO_ARM:
            switch(ThisEvent.EventParam)
            {
                case start:
                    PWMOperate_SetDutyOnChannel(4,5);
                    break;
                
                case stop:
                    PWMOperate_SetDutyOnChannel(15,5);
                    break;
            }
            break;
            
        case SERVO_BUCKET:
            switch(ThisEvent.EventParam)
            {
                case start:
                    PWMOperate_SetDutyOnChannel(9,4);
                    break;
                
                case stop:
                    PWMOperate_SetDutyOnChannel(10,4);
                    break;
            }
            break;
            
        case DC_CONVEYOR:
            switch(ThisEvent.EventParam)
            {
                case start:
                    DC_CONVEYOR_OUTPUT = 1;
                    break;

                case stop:
                    DC_CONVEYOR_OUTPUT = 0;
                    break;
            }
            
            break;
            
//        case DC_WHEEL:
//            switch(ThisEvent.EventParam)
//            {
//                case start:
//                    DC_WHEEL_OUTPUT = 1;
//                    break;
//
//                case stop:
//                    DC_WHEEL_OUTPUT = 0;
//                    break;
//            }
//            
//            break;
            
        case ES_NEW_KEY:
        {
             switch(ThisEvent.EventParam)
            {
                 case 'q':
                 {
                    NewEvent.EventType = SERVO_INTAKE;
                    NewEvent.EventParam = start;
                    PostServoService(NewEvent);
                    break;
                 }
                 case 'w':
                 {
                    NewEvent.EventType = SERVO_INTAKE;
                    NewEvent.EventParam = stop;
                    PostServoService(NewEvent);
                    break;
                 }

                 case 'e':
                 {
                    DB_printf("In SERVO_ARM start\n");

                    NewEvent.EventType = SERVO_ARM;
                    NewEvent.EventParam = start;
                    PostServoService(NewEvent);                     
                     break;
                 }

                 case 'r':
                 {
                    DB_printf("In SERVO_ARM stop\n");

                    NewEvent.EventType = SERVO_ARM;
                    NewEvent.EventParam = stop;
                    PostServoService(NewEvent);                       
                     break;
                 }
                 
                 case 't':
                 {
                    NewEvent.EventType = SERVO_BUCKET;
                    NewEvent.EventParam = start;
                    PostServoService(NewEvent);  
                                DB_printf("In SERVO_BUCKET\n");

                     break;
                 }    
                 
                 case 'y':
                 {
                    NewEvent.EventType = SERVO_BUCKET;
                    NewEvent.EventParam = stop;
                    PostServoService(NewEvent);                       
                     break;
                 } 
                 
                 case 'u':
                 {
                    NewEvent.EventType = SERVO_INDICATOR;
                    NewEvent.EventParam = Green;
                    PostServoService(NewEvent);                       
                     break;
                 }    
                 
                 case 'v':
                 {
                    NewEvent.EventType = SERVO_INDICATOR;
                    NewEvent.EventParam = Blue;
                    PostServoService(NewEvent);                       
                     break;
                 }   
                 
                 case 'o':
                 {
                    NewEvent.EventType = DC_CONVEYOR;
                    NewEvent.EventParam = start;
                    PostServoService(NewEvent);                       
                     break;
                 }    
                 
                 case 'p':
                 {
                    NewEvent.EventType = DC_CONVEYOR;
                    NewEvent.EventParam = stop;
                    PostServoService(NewEvent);                       
                     break;
                 }            
//                 case 'k':
//                 {
//                    NewEvent.EventType = DC_WHEEL;
//                    NewEvent.EventParam = stop;
//                    PostServoService(NewEvent);                       
//                     break;
//                 }    
//                 
//                 case 'l':
//                 {
//                    NewEvent.EventType = DC_WHEEL;
//                    NewEvent.EventParam = stop;
//                    PostServoService(NewEvent);                       
//                     break;
//                 }                         
             }
            break;
        }
        default:
            // Unknown event
            break;
    }
    
    

    return ReturnEvent;
}

/*------------------------------- Private Functions -----------------------*/
static void SetServoDuty(uint8_t channel, uint8_t duty)
{
    if (channel >= 1 && channel <= 2 && duty <= 100)
    {
        PWMOperate_SetDutyOnChannel(duty, channel);
        DB_printf("Servo channel %d set to duty %d%%\n", channel, duty);
    }
}

static void SetServoPulseWidth(uint8_t channel, uint16_t pulseWidth)
{
    if (channel >= 1 && channel <= 2)
    {
        PWMOperate_SetPulseWidthOnChannel(pulseWidth, channel);
        DB_printf("Servo channel %d set to pulse width %d ticks\n", channel, pulseWidth);
    }
}
