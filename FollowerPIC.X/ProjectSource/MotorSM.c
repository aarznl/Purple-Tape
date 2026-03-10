/****************************************************************************
 Module
   MotorService.c

 Revision
   1.0.1

 Description
   This is a template file for implementing a simple service under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BeaconService.h"
#include "PIC32_AD_Lib.h"
#include "MotorSM.h"
#include "SPIFollowerService.h"
#include "EncoderService.h"
#include "dbprintf.h"
#include <stdlib.h>

/*----------------------------- Module Defines ----------------------------*/

#define AD_MAX 1023.0

#define T3_PERIOD 249 // period for 10kHz
#define PERIOD_90DEG 1900
#define FWD_PERIOD 2000

#define LINESENSE_PERIOD 100
//#define LINESENSE_PERIOD 2000
#define FWDBWD_PERIOD 1500

#define TAPE_IR_HIGH 750
#define TAPE_IR_LOW 200
#define DUTY_STEP_LEFT 2 // test for the duty cycle we want at each step
#define DUTY_STEP_RIGHT 2
#define LINE_SEEN 1
#define LINE_SEEN_TURN 1
//#define LINE_SEEN_BACK 1 // should be 1

#define FWDBWDnum 10

//defines for auto scan:
#define RB2_ANx (1<<4) // front left
#define RB3_ANx (1<<5) // front middle
#define RB12_ANx (1<<12) // front right
#define RB13_ANx (1<<11) // back left: between wheels on the left(far for counter)
#define RB15_ANx (1<<9) // back right: between wheels on the right(close for marker)

#define UPDATE_TIME 100
#define CONTROL_TIME 5


#define N0_LINE_FOLLOW 0
#define LINE_FOLLOW_FORWARD 1
#define LINE_FOLLOW_BACKWARD 2

#define ANGLE_DISTANCE_90 212 // millimeters

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static void PWMOutputs_Init(void);
static void OCRegs_Init(void);
static void InitLineSensors(void);

static void SetMotorSpeed(bool WhichMotor, MotorDirection_t WhichDirection, float DutyValue);
static void PrintLineSensorValues(void);
static void SetTargetRPM (int16_t LeftRPM, int16_t RightRPM);
static void LineFollowingControl(int16_t LineFollowingRPM);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static uint32_t LineSensorValues[5];
static volatile uint32_t BackLeft, BackRight, CenterLeft, CenterMiddle, CenterRight;
static volatile uint32_t LastBackRight, LastCenterMiddle;

static MotorDirection_t Motor1_Dir;
static MotorDirection_t Motor2_Dir;

static const float P_Constant = 0.1;
static const float I_Constant = 0.03;

static const float Kp = 0.01;
static const float Kd = 0.2;
static const float Ki = 0;

static volatile int16_t TargetLeftRPM = 0;
static volatile int16_t CurrentLeftRPM = 0;
static volatile int16_t CurrentLeftError = 0;
static volatile int16_t TotalLeftError = 0;
static volatile int16_t LeftDuty = 0;

static volatile int16_t CurrentRightRPM = 0;
static volatile int16_t TargetRightRPM = 0;
static volatile int16_t CurrentRightError = 0;
static volatile int16_t TotalRightError = 0;
static volatile int16_t RightDuty = 0;

static volatile uint16_t counter = 0;

static MotorSMState_t CurrentState;
static FollowState_t FollowState;

static uint8_t LineSeenCounter;
static uint16_t  LastSeen;


static uint8_t LineControlStatus;
static volatile int32_t PastLineError = 0;
static volatile int32_t TotalLineError = 0;
static volatile int32_t RPMChange = 0;

static volatile bool Checking4BackRight = false;
static volatile bool Checking4CenterMiddle = false;


/*------------------------------ Module Code ------------------------------*/

bool InitMotorSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  CurrentState = InitMotorSMState;
  FollowState = AlignFromBeacon;
  
  PWMOutputs_Init();
  OCRegs_Init();
  
  InitLineSensors();
  uint16_t AutoScanBitMask = RB2_ANx | RB3_ANx | RB12_ANx | RB13_ANx | RB15_ANx;
  ADC_ConfigAutoScan(AutoScanBitMask);
  
  
  ADC_MultiRead(LineSensorValues);
  CenterLeft = LineSensorValues[0];
  CenterMiddle = LineSensorValues[1];
  CenterRight = LineSensorValues[2];
  BackLeft = LineSensorValues[3];
  BackRight = LineSensorValues[4];
  
         
  Motor1_Dir = Stopped;
  Motor2_Dir = Stopped;
  
  
  LastSeen = 0;
  
  LineControlStatus = N0_LINE_FOLLOW;
  
    ES_Timer_InitTimer(ADC_UPDATE_TIMER, UPDATE_TIME);
//  /********************************************
//   in here you write your initialization code
//   *******************************************/

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

bool PostMotorSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunMotorSM(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
    /********************************************
     in here you write your service code
     *************
     ******************************/
    switch (ThisEvent.EventType)
    {
        case ES_STOP:
        {
            SetTargetRPM(0, 0);
            LineControlStatus = N0_LINE_FOLLOW;
            UpdateStatus(CMD_DONE);
            DB_printf("Motors stopped\n");
            break;
        }        
        case ES_DRIVEFORWARD:
        {
            SetTargetRPM(50, 50);
            LineControlStatus = LINE_FOLLOW_FORWARD;
            DB_printf("Drive forward\n");
            ES_Timer_InitTimer(ADC_UPDATE_TIMER, UPDATE_TIME);
            break;
        }
        
        case ES_DRIVEFORWARD_5:
        {
            SetTargetRPM(30, 30);
            DriveDistance(50);
            DB_printf("Drive forward 5cm\n");
            LineControlStatus = LINE_FOLLOW_FORWARD;
            break;
        }
        
        case ES_DRIVEFORWARD_10:
        {
            SetTargetRPM(30, 30);
            DriveDistance(100);
            DB_printf("Drive forward 10cm\n");
            LineControlStatus = LINE_FOLLOW_FORWARD;
            
            break;
        } 
              
        case ES_DRIVEBACKWARD:
        {
            SetTargetRPM(-30, -30);
            LineControlStatus = N0_LINE_FOLLOW;
            DB_printf("Drive backward\n");
            break;
        }    
        
        case ES_DRIVEBACKWARD_5:
        {
            SetTargetRPM(-30, -30);
            DriveDistance(50);
            DB_printf("Drive backward 5cm\n");
            LineControlStatus = N0_LINE_FOLLOW;
            
            break;
        } 
        
        case ES_DRIVEBACKWARD_10:
        {
            SetTargetRPM(-15, -15);
            DriveDistance(100);
            DB_printf("Drive backward 10cm\n");
            LineControlStatus = N0_LINE_FOLLOW;
            
            break;
        } 
        
        case ES_DRIVEBACKWARD_20:
        {
            SetTargetRPM(-30, -30);
            DriveDistance(200);
            DB_printf("Drive backward 20cm\n");
            LineControlStatus = N0_LINE_FOLLOW;
            break;
        } 
        
        case ES_DRIVEBACKWARD_30:
        {
            SetTargetRPM(-15, -15);
            DriveDistance(275);
            DB_printf("Drive backward 30cm\n");
            LineControlStatus = N0_LINE_FOLLOW;
            break;
        }
        
        case ES_TURN90CW:
        {
            SetTargetRPM(15, -15);
            DriveDistance(ANGLE_DISTANCE_90);
            LineControlStatus = N0_LINE_FOLLOW;
            DB_printf("Turn 90 CW\n");
            break;
        }              
        
        case ES_TURN90CCW:
        {
            SetTargetRPM(-15, 15);
            DriveDistance(ANGLE_DISTANCE_90);
            LineControlStatus = N0_LINE_FOLLOW;
            DB_printf("Turn 90 CCW\n");
            break;
        }
        
        case ES_TURNCW:
        {
            SetTargetRPM(15, -15);
            LineControlStatus = N0_LINE_FOLLOW;
            DB_printf("Turn CW\n");
            break;
        }
        
        case ES_TURNCCW:
        {
            SetTargetRPM(-15, 15);
            LineControlStatus = N0_LINE_FOLLOW;
            DB_printf("Turn CCW\n");
            break;
        }
        
        case ES_CCW_TAPE:
        {
            SetTargetRPM(-15, 15);
            LineControlStatus = N0_LINE_FOLLOW;
            DB_printf("Turn CCW until see the tape\n");
            Checking4CenterMiddle = true;
            break;
        }
        
        case ES_CW_TAPE:
        {
            SetTargetRPM(15, -15);
            LineControlStatus = N0_LINE_FOLLOW;
            DB_printf("Turn CW until see the tape\n");
            Checking4CenterMiddle = true;
            break;
        }
        
        case ES_FWD_TAPE:
        {
            SetTargetRPM(30, 30);
            LineControlStatus = LINE_FOLLOW_FORWARD;
            DB_printf("Go fwd until see the tape\n");
            Checking4BackRight = true;
            break;
        }
        
        case ES_BWD_TAPE:
        {
            SetTargetRPM(-30, -30);
            LineControlStatus = N0_LINE_FOLLOW;
            DB_printf("Go bwd until see the tape\n");
            Checking4BackRight = true;
            break;
        }
        
        case ES_TURN45CW:
        {
            SetTargetRPM(15, -15);
            DriveDistance(ANGLE_DISTANCE_90/2);
            LineControlStatus = N0_LINE_FOLLOW;
            DB_printf("Turn 45 CW\n");

            break;
        }
         
        case ES_TURN180CW:
        {
            SetTargetRPM(15, -15);
            DriveDistance(ANGLE_DISTANCE_90*2);
            LineControlStatus = N0_LINE_FOLLOW;
            DB_printf("Turn 180 CW\n");

            break;
        }
        
        case ES_TURN225CW:
        {
            SetTargetRPM(15, -15);
            DriveDistance(ANGLE_DISTANCE_90*2+40); // Not exactly 225 degrees, slightly less
            LineControlStatus = N0_LINE_FOLLOW;
            DB_printf("Turn 225 CW\n");

            break;
        }
        
        case ES_TIMEOUT:
        {
            switch (ThisEvent.EventParam)
            {
                case ADC_UPDATE_TIMER:
                {
                    ES_Timer_InitTimer(ADC_UPDATE_TIMER, CONTROL_TIME);
                    ADC_MultiRead(LineSensorValues);
                    CenterLeft = LineSensorValues[0];
                    CenterMiddle = LineSensorValues[1];
                    CenterRight = LineSensorValues[2];
                    BackLeft = LineSensorValues[3];
                    BackRight = LineSensorValues[4];
                    if (LineControlStatus == LINE_FOLLOW_FORWARD) {
                        LineFollowingControl(35);
                    }
                    break;
                }
            }
                        
            break;
        }
        
        case ES_NEW_KEY:
        {
            ES_Event_t KeyEvent;
            if (ThisEvent.EventParam == '1') {
                KeyEvent.EventType = ES_STOP;
                PostMotorSM(KeyEvent);
            } else if (ThisEvent.EventParam == '2') {
                KeyEvent.EventType = ES_DRIVEFORWARD;
                PostMotorSM(KeyEvent);
            } else if (ThisEvent.EventParam == '3') {
                KeyEvent.EventType = ES_DRIVEBACKWARD;
                PostMotorSM(KeyEvent);
            } else if (ThisEvent.EventParam == '4') {
                KeyEvent.EventType = ES_TURN90CW;
                PostMotorSM(KeyEvent);
            } else if (ThisEvent.EventParam == '5') {
                KeyEvent.EventType = ES_TURN90CCW;
                PostMotorSM(KeyEvent);
            } else if (ThisEvent.EventParam == '6') {
                PrintLineSensorValues();
            }
            break;
        }
    }
        
    return ReturnEvent;
}

MotorSMState_t QueryMotorSM(void)
{
    return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/
static void SetTargetRPM (int16_t LeftRPM, int16_t RightRPM)
{
    TargetLeftRPM = LeftRPM;
    TargetRightRPM = RightRPM;
}

void Control (int16_t CurrentLeftRPM, int16_t CurrentRightRPM)
{
    
    CurrentLeftError = abs(TargetLeftRPM) - CurrentLeftRPM;
    TotalLeftError = TotalLeftError + CurrentLeftError;
    LeftDuty = P_Constant * (float)CurrentLeftError + I_Constant * (float)TotalLeftError;
    if (LeftDuty > 100) {
        TotalLeftError = TotalLeftError - CurrentLeftError;
        LeftDuty = 100;
    } else if (LeftDuty < 0) {
        TotalLeftError = TotalLeftError - CurrentLeftError;
        LeftDuty = 0;
    }
    
    CurrentRightError = abs(TargetRightRPM) - CurrentRightRPM;
    TotalRightError = TotalRightError + CurrentRightError;
    RightDuty = P_Constant * (float)CurrentRightError + I_Constant * (float)TotalRightError;
    
    if (RightDuty > 100) {
        TotalRightError = TotalRightError - CurrentRightError;
        RightDuty = 100;
    } else if (RightDuty < 0) {
        TotalRightError = TotalRightError - CurrentRightError;
        RightDuty = 0;
    }
    
    if (TargetLeftRPM > 0) {
        SetMotorSpeed(LEFT, Forward, LeftDuty);

    } else if (TargetLeftRPM < 0) {
        SetMotorSpeed(LEFT, Backward, LeftDuty);

    } else if (TargetLeftRPM == 0) {
        SetMotorSpeed(LEFT, Stopped, 0);
    }    
    
    if (TargetRightRPM > 0) {
        SetMotorSpeed(RIGHT, Forward, RightDuty);

    } else if (TargetRightRPM < 0) {
//        RightDuty = RightDuty * -1;
        SetMotorSpeed(RIGHT, Backward, RightDuty);

    } else if (TargetRightRPM == 0) {
        SetMotorSpeed(RIGHT, Stopped, 0);
    }
    
//    if (counter == 500) {
//        counter = 0;
//        //DB_printf("CurrentRight: %d.\n", CurrentLeftRPM);
//        //DB_printf("CurrentLeft: %d.\n", CurrentRightRPM);
//    }
//    counter++;

}

static void SetMotorSpeed(bool WhichMotor, MotorDirection_t WhichDirection, float DutyValue)
{
    float dutyPct = ((float)DutyValue/100.0); // Calculate new duty pct 
    uint32_t periodVal = T3_PERIOD*dutyPct; // Calculate new period value
    switch (WhichDirection)
    {
        case Forward:
        {
            if (WhichMotor == 0)
            {
                OC1RS = 0;
                OC2RS = periodVal;
                Motor1_Dir = Forward;
            }
            else
            {
                OC3RS = 0;
                OC4RS = periodVal;
                Motor2_Dir = Forward;
            }
            break;
        }
        case Backward:
        {
            if (WhichMotor == 0)
            {
                OC1RS = periodVal;
                OC2RS = 0;
                Motor1_Dir = Backward;
            }
            else
            {
                OC3RS = periodVal;
                OC4RS = 0;
                Motor2_Dir = Backward;
            }
            break;
        }
        case Stopped:
        {
            if (WhichMotor == 0)
            {
                OC1RS = 0;
                OC2RS = 0;
                Motor1_Dir = Stopped;
            }
            else
            {
                OC3RS = 0;
                OC4RS = 0;
                Motor2_Dir = Stopped;
            }
            break;
        }
    }
}

static void PrintLineSensorValues(void)
{
    ES_Timer_InitTimer(ADC_UPDATE_TIMER, CONTROL_TIME);
    DB_printf("BackLeft: %d\n", BackLeft);
    DB_printf("BackRight: %d\n", BackRight);
    DB_printf("CenterLeft: %d\n", CenterLeft);
    DB_printf("CenterMiddle: %d\n", CenterMiddle);
    DB_printf("CenterRight: %d\n", CenterRight);
    DB_printf("--------------------------\n");
}

static void LineFollowingControl(int16_t LineFollowingRPM)
{ 
    int32_t CurrentLineError = CenterLeft-CenterRight;
    TotalLineError = TotalLineError + CurrentLineError;
    RPMChange = ((float)CurrentLineError*Kp+((float)CurrentLineError-(float)PastLineError)*Kd+(float)TotalLineError*Ki);
    int16_t NewTargetLeft = LineFollowingRPM - RPMChange;
    int16_t NewTargetRight = LineFollowingRPM + RPMChange;
    if (NewTargetLeft > 60)
    {
        NewTargetLeft = 60;
        TotalLineError = TotalLineError - CurrentLineError;
    }
    else if (NewTargetLeft < 5)
    {
        NewTargetLeft = 5;
        TotalLineError = TotalLineError - CurrentLineError;
    }
    
    if (NewTargetRight > 60) {
        NewTargetRight = 60;
        TotalLineError = TotalLineError - CurrentLineError;
    } else if (NewTargetRight < 5) {
        NewTargetRight = 5;
        TotalLineError = TotalLineError - CurrentLineError;
    }
    
    PastLineError = CurrentLineError;

    SetTargetRPM(NewTargetLeft, NewTargetRight);
    
//    if (counter == 10) {
//        counter = 0;
//        DB_printf("RPMCHange: %d.\n", RPMChange);
//        DB_printf("CurrentError: %d.\n", CurrentLineError);
//        DB_printf("NewLeft: %d.\n", NewTargetLeft);
//        DB_printf("NewRight: %d.\n", NewTargetRight);
//    }
//    
//    counter++;
}

// Event checker functions for line sensors
bool Check4BackRightLine(void)
{
    bool ReturnVal = false;
    if (Checking4BackRight == true)
    {
        if (LastBackRight < TAPE_IR_HIGH && BackRight > TAPE_IR_HIGH) {
            UpdateStatus(BACK_RIGHT_DETECT);
            ReturnVal = true;
            Checking4BackRight = false;
//
        }
    }
    LastBackRight = BackRight;
    return ReturnVal;
}

bool Check4CenterMiddleLine(void)
{
    bool ReturnVal = false;
    if (Checking4CenterMiddle == true)
    {
        if (LastCenterMiddle < TAPE_IR_HIGH && CenterMiddle > TAPE_IR_HIGH) {
            UpdateStatus(TAPE_ALIGNED);
            ReturnVal = true;
            Checking4CenterMiddle = false;
        }
    }
    LastCenterMiddle = CenterMiddle;
    return ReturnVal;
}


static void PWMOutputs_Init(void)
{
    // OC1: SPI_RPB4
    TRISBbits.TRISB4 = 0; //output
    //no analog for b4
    RPB4R = 0b0101; //pps mapping for OC1
    
    // OC2: SPI_RPB11
    TRISBbits.TRISB11 = 0; //output
    //no analog for b11
    RPB11R = 0b0101; //pps mapping for OC2
    
    // OC3: SPI_RPA3
    TRISAbits.TRISA3 = 0; //output
    //no analog for a3
    RPA3R = 0b0101; //pps mapping for OC3
    
    // OC4: SPI_RPA4
    TRISAbits.TRISA4 = 0; //output
    // no analog for a4
    RPA4R = 0b0101; //pps mapping for OC4
    
}

static void OCRegs_Init(void)
{
    T3CONbits.ON = 0; // Timer3 off
    T3CONbits.TCS = 0; // PBClk as source
    T3CONbits.TCKPS = 0b011; // divide PBClk by 8 (prescale 8)
    TMR3 = 0;
    PR3 = T3_PERIOD; 
    
    // OC1 INIT:
    OC1CONbits.ON = 0; // OC1 off
    OC1CON = 0;
    OC1CONbits.OCTSEL = 1; // Assign timer 3
    OC1CONbits.OCM = 0b110; // PWM mode, edge-aligned
    OC1RS = 0; // 0% duty cycle
    OC1R = OC1RS;
    OC1CONbits.ON = 1; // OC1 on
    
    // OC2 INIT:
    OC2CONbits.ON = 0; // OC2 off
    OC2CON = 0;
    OC2CONbits.OCTSEL = 1; // Assign timer 3
    OC2CONbits.OCM = 0b110; // PWM mode, edge-aligned
    OC2RS = 0; // 0% duty cycle
    OC2R = OC2RS;
    OC2CONbits.ON = 1; // OC2 on
    
    // OC3 INIT:
    OC3CONbits.ON = 0; // OC3 off
    OC3CON = 0;
    OC3CONbits.OCTSEL = 1; // Assign timer 3
    OC3CONbits.OCM = 0b110; // PWM mode, edge-aligned
    OC3RS = 0; // 0% duty cycle
    OC3R = OC3RS;
    OC3CONbits.ON = 1; // OC3 on
    
    // OC4 INIT:
    OC4CONbits.ON = 0; // OC4 off
    OC4CON = 0;
    OC4CONbits.OCTSEL = 1; // Assign timer 3
    OC4CONbits.OCM = 0b110; // PWM mode, edge-aligned
    OC4RS = 0; // 0% duty cycle
    OC4R = OC4RS;
    OC4CONbits.ON = 1; // OC4 on
      
    T3CONbits.ON = 1; // Timer3 on
}

static void InitLineSensors(void)
{
    // line sensors must be on analog pins
    // sensor pins we chose: B2, B3, B12, B13, B15
    
    // SENSOR 1: B2
    TRISBbits.TRISB2 = 1; //configure as input
    ANSELBbits.ANSB2 = 1; //enable analog
    
    // SENSOR 2: B3
    TRISBbits.TRISB3 = 1; //configure as input
    ANSELBbits.ANSB3 = 1; //enable analog
    
    // SENSOR 3: B12
    TRISBbits.TRISB12 = 1; //configure as input
    ANSELBbits.ANSB12 = 1; //enable analog
    
    // SENSOR 4: B13
    TRISBbits.TRISB13 = 1; //configure as input
    ANSELBbits.ANSB13 = 1; //enable analog
    
    // SENSOR 5: B15
    TRISBbits.TRISB15 = 1; //configure as input
    ANSELBbits.ANSB15 = 1; //enable analog
}


/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

