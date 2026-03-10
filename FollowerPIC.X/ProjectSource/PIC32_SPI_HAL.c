/*----------------------------- Include Files -----------------------------*/
#include <xc.h>
#include <stdbool.h> 
#include "PIC32_SPI_HAL.h"

/*--------------------------- External Variables --------------------------*/

/*----------------------------- Module Defines ----------------------------*/
// this is based on a 13 bit (max=8191) BRG register and 20MHz (50ns) PBCLK
#define MAX_SPI_PERIOD  ((8191+1)*2*50)
#define MAP_SS1 0b0011
#define MAP_SS2 0b0100
#define MAP_SDO1 0b0011
#define MAP_SDO2 0b0100

#define PBCLK_MHz 20 //
/*------------------------------ Module Types -----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
static void selectModuleRegisters(SPI_Module_t WhichModule);
static bool isSPI_ModuleLegal( SPI_Module_t WhichModule);
static bool isSSPinLegal(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin);
static bool isSDxPinLegal(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin); // NOTE: this was changed to include SDI as well as SDO

/*---------------------------- Module Variables ---------------------------*/
  // these will allow us to reference both SPI1 & SPI2 through these pointers
static volatile __SPI1CONbits_t * pSPICON;   
static volatile __SPI1CON2bits_t * pSPICON2;
static volatile uint32_t * pSPIBRG;
static volatile uint32_t * pSPIBUF;

// these are the output mapping registers indexed by the SPI_PinMap_t value
static volatile uint32_t * const outputMapRegisters[] = { &RPA0R, &RPA1R, 
                      &RPA2R, &RPA3R, &RPA4R, 
                      &RPB0R, &RPB1R, &RPB2R, &RPB3R, &RPB4R, &RPB5R, 
                      &RPB6R, &RPB7R, &RPB8R, &RPB9R, &RPB10R, &RPB11R, &RPB12R,
                      &RPB13R, &RPB14R, &RPB15R 
};

// these are the TRISxSET registers indexed by the SPI_PinMap_t value
static volatile uint32_t * const setTRISRegisters[] = { &TRISASET, &TRISASET,
            &TRISASET, &TRISASET, &TRISASET,
            &TRISBSET, &TRISBSET, &TRISBSET, &TRISBSET, &TRISBSET, &TRISBSET,
            &TRISBSET, &TRISBSET, &TRISBSET, &TRISBSET, &TRISBSET, &TRISBSET, 
            &TRISBSET, &TRISBSET, &TRISBSET, &TRISBSET
};

// these are the TRISxCLR registers indexed by the SPI_PinMap_t value
static volatile uint32_t * const clrTRISRegisters[] = { &TRISACLR, &TRISACLR,
            &TRISACLR, &TRISACLR, &TRISACLR,
            &TRISBCLR, &TRISBCLR, &TRISBCLR, &TRISBCLR, &TRISBCLR, &TRISBCLR,
            &TRISBCLR, &TRISBCLR, &TRISBCLR, &TRISBCLR, &TRISBCLR, &TRISBCLR, 
            &TRISBCLR, &TRISBCLR, &TRISBCLR, &TRISBCLR
};

// these are the ANSELxCLR registers indexed by the SPI_PinMap_t value
static volatile uint32_t * const clrANSELRegisters[] = { &ANSELACLR, &ANSELACLR,
         &ANSELACLR, &ANSELACLR, &ANSELACLR,
         &ANSELBCLR, &ANSELBCLR, &ANSELBCLR, &ANSELBCLR, &ANSELBCLR, &ANSELBCLR,
         &ANSELBCLR, &ANSELBCLR, &ANSELBCLR, &ANSELBCLR, &ANSELBCLR, &ANSELBCLR, 
         &ANSELBCLR, &ANSELBCLR, &ANSELBCLR, &ANSELBCLR
};

// these are the bit positions indexed by the SPI_PinMap_t value
static uint32_t const mapPinMap2BitPosn[] = { 1<<0, 1<<1,
         1<<2, 1<<3, 1<<4,
         1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5,
         1<<6, 1<<7, 1<<8, 1<<9, 1<<10, 1<<11, 
         1<<12, 1<<13, 1<<14, 1<<15
};

// these are the INT pin mapping constants indexed by the SPI_PinMap_t value
static uint32_t const mapPinMap2INTConst[] = { 0b0000/*RA0*/, 0b0000/*RA1*/,
         0b0000/*RA2*/, 0b0000/*RA3*/, 0b0010/*RA4*/,
         0b0010/*RB0*/,0b0010/*RB1*/, 0b0100/*RB2*/, 0b0001/*RB3*/, 
         0b0010/*RB4*/, 0b0001/*RB5*/, 0b0001/*RB6*/, 0b0100/*RB7*/, 
         0b0100/*RB8*/, 0b0100/*RB9*/, 0b0011/*RB10*/, 0b0011/*RB11*/, 
         0/*RB12*/, 0b0011/*RB13*/, 0b0001/*RB14*/, 0b0011/*RB15*/
};

static SPI_PinMap_t const LegalSSPins[][5] = {{ SPI_RPA0, SPI_RPB3, SPI_RPB4, 
                                             SPI_RPB7,SPI_RPB15 },
                                             { SPI_RPA3, SPI_RPB0, SPI_RPB9, 
                                             SPI_RPB10,SPI_RPB14 }
};

static SPI_PinMap_t const LegalSDxPins[][5] = { {SPI_RPA1, SPI_RPB5, SPI_RPB1, 
                                              SPI_RPB11, SPI_RPB8}, 
                                              {SPI_RPA2, SPI_RPB6, SPI_RPA4, 
                                              SPI_RPB13, SPI_RPB2 }
};

bool SPISetup_BasicConfig(SPI_Module_t WhichModule)
{
  bool ReturnVal = true;
   
  // Make sure that we have a legal module specified
  if ( false == isSPI_ModuleLegal(WhichModule))
  {
      ReturnVal = false;
  }
  else // Legal module so set it up
  {
    selectModuleRegisters(WhichModule); 
    pSPICON->ON = 0;        // Disable the selected SPI Module
    pSPICON->MCLKSEL = 0;   // Configure the SPI clock to be based on PBCLK
    pSPICON->FRMEN = 0;     // Disable the Framed mode
    pSPICON2->AUDEN = 0;    // Disable the Audio mode
  }   
  return ReturnVal;
}

bool SPISetup_SetFollower(SPI_Module_t WhichModule)
{
  bool ReturnVal = true;
  if (isSPI_ModuleLegal(WhichModule))
  {
      uint16_t clockPin;
      switch (WhichModule)
      { // conditionals for setting SMP bit
          case SPI_SPI1:
              clockPin = SPI_RPB14; // RB14 always clock for SPI1
              break;
          case SPI_SPI2:
              clockPin = SPI_RPB15; // RB15 always clock for SPI2
              break;
          default: // SHOULD NEVER HAPPEN
              ReturnVal = false;
              return ReturnVal;
      }
      pSPICON -> MSTEN = 0; // follower mode
//      pSPICON -> SSEN = 1; // enable automatic driving of SS line
      *setTRISRegisters[clockPin] = mapPinMap2BitPosn[clockPin]; // configure clockPin as input by setting TRIS
  }
  else // if not legal module
  {
      ReturnVal = false;
  }
  return ReturnVal;
}

bool SPISetup_SetLeader(SPI_Module_t WhichModule, SPI_SamplePhase_t WhichPhase)
{
  bool ReturnVal = true;
  if (isSPI_ModuleLegal(WhichModule))
  {
      pSPICON -> MSTEN = 1; // leader mode
      switch (WhichPhase)
      { // conditionals for setting SMP bit
          case SPI_SMP_MID: // sample middle
              pSPICON->SMP = 0;
              break;
          case SPI_SMP_END: // sample end
              pSPICON->SMP = 1;
              break;
          default: // invalid phase argument
              ReturnVal = false;
      }
      uint8_t clockPin;
      switch (WhichModule)
      { // clockPin is always 14 or 15, it just depends on the SPI module
          case SPI_SPI1:
              clockPin = SPI_RPB14; // clock is always RB14 for SPI1
              break;
          case SPI_SPI2:
              clockPin = SPI_RPB15; // clock is always RB15 for SPI2
              break;
          default: // SHOULD NEVER HAPPEN
              ReturnVal = false;
              return ReturnVal;
      }
      *clrANSELRegisters[clockPin] = mapPinMap2BitPosn[clockPin]; // clear analog for clockPin
      *clrTRISRegisters[clockPin] = mapPinMap2BitPosn[clockPin]; // configure as output by clearing TRIS
  }
  else // module not legal
  {
      ReturnVal = false;
  }
  return ReturnVal;
}

bool SPISetup_SetBitTime(SPI_Module_t WhichModule, uint32_t SPI_ClkPeriodIn_ns)
{
  bool ReturnVal = true;
  // Make sure that we have a legal module specified and legal clock period
    if ( false == isSPI_ModuleLegal(WhichModule) || 
            SPI_ClkPeriodIn_ns > MAX_SPI_PERIOD)
    {
        ReturnVal = false;
    }else // Legal module so set it up
    {  
        selectModuleRegisters(WhichModule);

        uint32_t baudRateRegister = (0.01*SPI_ClkPeriodIn_ns)-1; // Calculate baud rate
        *pSPIBRG = baudRateRegister; // Set register to calculated value
    }
    return ReturnVal;
}

bool SPISetup_MapSSInput(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin)
{
  // I think we still don't need this for lab 8
  // because we will not be setting up the pic in slave/follower mode
  // but if I have time I'll implement it anyway - tay
  bool ReturnVal = true;
  
  // Make sure that we have a legal module specified & legal pin
  if ( (false == isSPI_ModuleLegal(WhichModule)) || 
       (false == isSSPinLegal(WhichModule, WhichPin)) )
  {
    ReturnVal = false;
  }else 
  { // Legal module  & pin so set try setting it up
    selectModuleRegisters(WhichModule);
    if (0 == pSPICON->MSTEN)  // configured in follower mode?
    {
      if (SPI_NO_PIN == WhichPin)
      {
        pSPICON->MSSEN = 0; // disable SS
      }else //there is an SS pin so map it
      {
        pSPICON->MSSEN = 1; // enable SS
        // set the TRIS bit to make it an input
        *setTRISRegisters[WhichPin] = mapPinMap2BitPosn[WhichPin];
        // clear the ANSEL bit to disable analog on the pin
        *clrANSELRegisters[WhichPin] = mapPinMap2BitPosn[WhichPin];
            
        if (SPI_SPI1 == WhichModule)
        {
          SS1R = mapPinMap2INTConst[WhichPin];
        }
        else  
        {   // must be SPI2
          SS2R = mapPinMap2INTConst[WhichPin];
        }
      }
    }else // not in follower mode
    {
      ReturnVal = false; // then we can't config an SS output
    }
  }
  return ReturnVal;
}

bool SPISetup_MapSSOutput(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin)
{
  bool ReturnVal = true;
  
  // Make sure that we have a legal module specified & legal pin
  if ( (false == isSPI_ModuleLegal(WhichModule)) || 
       (false == isSSPinLegal(WhichModule, WhichPin)) )
  {
    ReturnVal = false;
  }else 
  { // Legal module  & pin so set try setting it up
    selectModuleRegisters(WhichModule);
    if (1 == pSPICON->MSTEN)  // configured in Leader mode?
    {
      if (SPI_NO_PIN == WhichPin)
      {
        pSPICON->MSSEN = 0; // disable SS
      }else //there is an SS pin so map it
      {
        pSPICON->MSSEN = 1; // enable SS
        // clear the TRIS bit to make it an output
        *clrTRISRegisters[WhichPin] = mapPinMap2BitPosn[WhichPin];
        // clear the ANSEL bit to disable analog on the pin
        *clrANSELRegisters[WhichPin] = mapPinMap2BitPosn[WhichPin];
            
        if (SPI_SPI1 == WhichModule)
        {
          *outputMapRegisters[WhichPin] = MAP_SS1; // Map SS to chosen pin
          // set up to use INT4 to capture the rising edge of SS
          INTCONbits.INT4EP = 1;            // set for rising edge sensitivity
          IFS0CLR = _IFS0_INT4IF_MASK;      // clear any pending flag
          INT4R = mapPinMap2INTConst[WhichPin];  // map INT4 to SS as well
        }else  
        {   // must be SPI2 so set up INT1
          *outputMapRegisters[WhichPin] = MAP_SS2; // Map SS to chosen pin
          // set up to use INT1 to capture the rising edge of SS
          INTCONbits.INT1EP = 1;            // set for rising edge sensitivity
          IFS0CLR = _IFS0_INT1IF_MASK;      // clear any pending flag
          INT1R = mapPinMap2INTConst[WhichPin];  // map INT1 to SS as well
        }
      }
    }else // not in Leader mode
    {
      ReturnVal = false; // then we can't config an SS output
    }
  }
  return ReturnVal;
}

bool SPISetup_MapSDInput(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin)
{
    bool ReturnVal = true;
    
    if (isSPI_ModuleLegal(WhichModule) && isSDxPinLegal(WhichModule, WhichPin))
    {
        *setTRISRegisters[WhichPin] = mapPinMap2BitPosn[WhichPin]; // configure as input by setting TRIS
        *clrANSELRegisters[WhichPin] = mapPinMap2BitPosn[WhichPin]; // disable analog
        switch (WhichModule)
        { // pps mapping below
            case SPI_SPI1:
                SDI1R = mapPinMap2INTConst[WhichPin];
                break;
            case SPI_SPI2:
                SDI2R = mapPinMap2INTConst[WhichPin];
                break;
            default: // SHOULD NEVER HAPPEN
                ReturnVal = false;
        }
    }
    else // either not legal module or not legal pin
    {
        ReturnVal = false;
    }
    return ReturnVal;
}

bool SPISetup_MapSDOutput(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin)
{
  bool ReturnVal = true;
  // Your Code goes here :-)
  if (isSPI_ModuleLegal(WhichModule))
  {
      if (isSDxPinLegal(WhichModule, WhichPin))
      {
          *clrTRISRegisters[WhichPin] = mapPinMap2BitPosn[WhichPin]; // configure as output by clearing TRIS
          *clrANSELRegisters[WhichPin] = mapPinMap2BitPosn[WhichPin]; // disable analog
          switch (WhichModule)
          { // PPS mapping below
              case SPI_SPI1: // if SPI1
                  *outputMapRegisters[WhichPin] = MAP_SDO1;
                  break;
              case SPI_SPI2: // if SPI2
                  *outputMapRegisters[WhichPin] = MAP_SDO2;
                  break;
              default: // SHOULD NEVER HAPPEN
                  ReturnVal = false;
          }
      }
      else // pin not legal
      {
          ReturnVal = false;
      }
  }
  else // module not legal
  {
      ReturnVal = false;
  }
  return ReturnVal;
}

bool SPISetup_SetClockIdleState(SPI_Module_t WhichModule, SPI_Clock_t WhichState)
{
  bool ReturnVal = true;
  // Your Code goes here :-)
  if (isSPI_ModuleLegal(WhichModule))
  {
      switch (WhichState)
      { // conditionals for setting clock polarity
          case SPI_CLK_LO:
              pSPICON -> CKP = 0;
              break;
          case SPI_CLK_HI:
              pSPICON -> CKP = 1;
              break;
          default: // invalid state input argument
              ReturnVal = false;
      }
  }
  else // module not legal
  {
      ReturnVal = false;
  }
  return ReturnVal;
  
}

bool SPISetup_SetActiveEdge(SPI_Module_t WhichModule, SPI_ActiveEdge_t WhichEdge)
{
  bool ReturnVal = true;
  // Your Code goes here :-)
  if (isSPI_ModuleLegal(WhichModule))
  {
      switch (WhichEdge)
      { // conditionals for setting CKE bit
          case SPI_SECOND_EDGE:
              pSPICON -> CKE = 0;
              break;
          case SPI_FIRST_EDGE:
              pSPICON -> CKE = 1;
              break;
          default: // invalid edge input argument
              ReturnVal = false;
      }
  }
  else // module not legal
  {
      ReturnVal = false;
  }
  return ReturnVal;
  
}

bool SPISetup_SetXferWidth(SPI_Module_t WhichModule, SPI_XferWidth_t DataWidth)
{
  bool ReturnVal = true;
  // Your Code goes here :-)
  if (isSPI_ModuleLegal(WhichModule))
  {
      switch (DataWidth)
      {
          case SPI_8BIT: // setup for 8bit
              pSPICON -> MODE16 = 0;
              pSPICON -> MODE32 = 0;
              break;
          case SPI_16BIT:  // setup for 16bit
              pSPICON -> MODE16 = 1;
              pSPICON -> MODE32 = 0;
              break;
          case SPI_32BIT:  // setup for 32bit
              pSPICON -> MODE16 = 0;
              pSPICON -> MODE32 = 1;
              break;
          default: // invalid data width input argument
              ReturnVal = false;
      }
  }
  else // module not legal
  {
      ReturnVal = false;
  }
  return ReturnVal;  
}

bool SPISetEnhancedBuffer(SPI_Module_t WhichModule, bool IsEnhanced)
{
  bool ReturnVal = true;
  // Your Code goes here :-)
  if (isSPI_ModuleLegal(WhichModule))
  {
      pSPICON -> ENHBUF = IsEnhanced; // set if 1, clear if 0
  }
  else // module not legal
  {
      ReturnVal = false;
  }
  return ReturnVal;
}

bool SPISetup_DisableSPI(SPI_Module_t WhichModule)
{
  bool ReturnVal = true;
  // Your Code goes here :-)
  if (isSPI_ModuleLegal(WhichModule))
  {
      pSPICON -> ON = 0; // disables SPI
  }
  else // module not legal
  {
      ReturnVal = false;
  }
  return ReturnVal;
  
}

bool SPISetup_EnableSPI(SPI_Module_t WhichModule)
{
  bool ReturnVal = true;
  // Your Code goes here :-)
  if (isSPI_ModuleLegal(WhichModule))
  {
      pSPICON -> ON = 1; // enables SPI
  }
  else // module not legal
  {
      ReturnVal = false;
  }
  return ReturnVal;
  
}

void SPIOperate_SPI1_Send8(uint8_t TheData)
{
    if (SPI1CONbits.MODE32) // 32-bit mode
    {
        SPI1BUF = (uint32_t)TheData;
    }
    else if (SPI1CONbits.MODE16) // 16-bit mode
    {
        SPI1BUF = (uint16_t)TheData;
    }
    else // 8-bit mode
    {
        SPI1BUF = TheData;
    }
}

void SPIOperate_SPI1_Send16( uint16_t TheData)
{
    if (SPI1CONbits.MODE32) // 32-bit mode
    {
        SPI1BUF = (uint32_t)TheData;
    }
    else if (SPI1CONbits.MODE16) // 16-bit mode
    {
        SPI1BUF = TheData;
    }
    else // 8-bit mode
    {
        SPIOperate_SPI1_Send8(TheData >> 8); // first 8 bits
        SPIOperate_SPI1_Send8(TheData & 0xFF); // last 8 bits
    }
}

void SPIOperate_SPI1_Send32(uint32_t TheData)
{

    if (SPI1CONbits.MODE32) // 32-bit mode
    {
        SPI1BUF = (uint32_t)TheData;
    }
    else if (SPI1CONbits.MODE16) // 16-bit mode
    {
        SPI1BUF = TheData;
    }
    else // 8-bit mode
    {
        SPIOperate_SPI1_Send8(TheData >> 24); // first 8 bits
        SPIOperate_SPI1_Send8(TheData >> 16); // next 8 bits
        SPIOperate_SPI1_Send8(TheData >> 8); // next 8 bits
        SPIOperate_SPI1_Send8(TheData & 0xFF); // last 8 bits
    }
}

void SPIOperate_SPI1_Send8Wait(uint8_t TheData)
{
    SPIOperate_SPI1_Send8(TheData);
    while (!SPIOperate_HasSS1_Risen()); // wait for SS1 rise
}

void SPIOperate_SPI1_Send16Wait( uint16_t TheData)
{
    SPIOperate_SPI1_Send16(TheData);
    while (!SPIOperate_HasSS1_Risen()); // wait for SS1 rise
}

void SPIOperate_SPI1_Send32Wait(uint32_t TheData)
{
    SPIOperate_SPI1_Send32(TheData);
    while (!SPIOperate_HasSS1_Risen()); // wait for SS1 rise
}

uint32_t SPIOperate_ReadData(SPI_Module_t WhichModule)
{
    volatile uint32_t data = 0;
    switch (WhichModule)
    {
        case SPI_SPI1:
            data = SPI1BUF;
            break;
        case SPI_SPI2:
            data = SPI2BUF;
            break;
        default:
            data = 0;
            break;
    }
    return data;
}

bool SPIOperate_HasSS1_Risen(void)
{
  bool ReturnVal = true;
  // NOT MY CODE - From Canvas
  if(IFS0bits.INT4IF == true)
  {  // INT4F is asserted at end of a transmission
    IFS0CLR = _IFS0_INT4IF_MASK;   // clear INT4F with special syntax
  }
  else
  {
    ReturnVal = false;
  }
  return ReturnVal;
}

bool SPIOperate_HasSS2_Risen(void)
{
    bool ReturnVal = true;
    // NOTE - assumes SS2 is connected to INT1
    if(IFS0bits.INT1IF == true)
    {  // INT1F is asserted at end of a transmission
      IFS0CLR = _IFS0_INT1IF_MASK;   // clear INT1F with special syntax
    }
    else
    {
      ReturnVal = false;
    }
    return ReturnVal;
}

static void selectModuleRegisters(SPI_Module_t WhichModule)
{
  if (SPI_SPI1 == WhichModule)
  {
    pSPICON = (__SPI1CONbits_t *)&SPI1CON;
    pSPICON2 = (__SPI1CON2bits_t *)&SPI1CON2;
    pSPIBRG = (volatile uint32_t *)&SPI1BRG;
    pSPIBUF = &SPI1BUF;
  }else if (SPI_SPI2 == WhichModule)
  {
    pSPICON = (__SPI1CONbits_t *)&SPI2CON;
    pSPICON2 = (__SPI1CON2bits_t *)&SPI2CON2;
    pSPIBRG = (volatile uint32_t *)&SPI2BRG;
    pSPIBUF = &SPI2BUF;
  }
}

static bool isSPI_ModuleLegal( SPI_Module_t WhichModule)
{
  // Your Code goes here :-)
    return (WhichModule == SPI_SPI1 || WhichModule == SPI_SPI2); // if the module is either one it returns true
}

static bool isSSPinLegal(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin)
{
  bool ReturnVal = false;
  uint8_t index;
  
  for( index = 0; 
       index <= ((sizeof(LegalSSPins[0])/sizeof(LegalSSPins[0][0]))-1);
       index++)
  {
    if (LegalSSPins[WhichModule][index] == WhichPin)
    {
      ReturnVal = true;
      break;
    }
  }
  return ReturnVal;
}

static bool isSDxPinLegal(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin)
{
  bool ReturnVal = false;
  uint8_t index;
  
  if (WhichPin == SPI_NO_PIN)
  {
      ReturnVal = true;
      return ReturnVal;
  }
  
  for( index = 0;
       index < (sizeof(LegalSDxPins[0]) / sizeof(LegalSDxPins[0][0]));
       index++) // iterate through list
  {
    if (LegalSDxPins[WhichModule][index] == WhichPin) // if there's a match, break out of the loop to return true
    {
      ReturnVal = true;
      break;
    }
  }
  return ReturnVal;
}

void Send16BitValsToBuffer(const short *arr16bits, size_t arraySize)
{
    for (uint8_t i = 0; i < arraySize; i++)
    {
        SPIOperate_SPI1_Send16(arr16bits[i]);
    }
}

void InitSPI(void){
    SPISetup_BasicConfig(SPI_SPI1);
    SPISetup_DisableSPI(SPI_SPI1);
    SPISetup_SetLeader(SPI_SPI1, SPI_SMP_MID);
    SPISetup_SetBitTime(SPI_SPI1, 10000);
    SPISetup_MapSSOutput(SPI_SPI1, SPI_RPA0);
    SPISetup_MapSDOutput(SPI_SPI1, SPI_RPA1);
    SPISetup_MapSDInput(SPI_SPI1, SPI_RPB5);
    SPISetup_SetClockIdleState(SPI_SPI1, SPI_CLK_HI);
    SPISetup_SetActiveEdge(SPI_SPI1, SPI_SECOND_EDGE);
    SPISetup_SetXferWidth(SPI_SPI1, SPI_8BIT);
    SPISetEnhancedBuffer(SPI_SPI1, true);
    SPISetup_EnableSPI(SPI_SPI1);
}
