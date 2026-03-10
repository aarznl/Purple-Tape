#ifndef PIC32_SPI_HAL
#define PIC32_SPI_HAL

#include <stdbool.h> 

typedef enum { 
    SPI_SPI1 = 0, 
    SPI_SPI2 = 1 
} SPI_Module_t;

typedef enum {
        SPI_RPA0 = 0,
        SPI_RPA1,
        SPI_RPA2,
        SPI_RPA3,
        SPI_RPA4,
        SPI_RPB0,
        SPI_RPB1,
        SPI_RPB2,
        SPI_RPB3,
        SPI_RPB4,
        SPI_RPB5,
        SPI_RPB6,
        SPI_RPB7,
        SPI_RPB8,
        SPI_RPB9,
        SPI_RPB10,
        SPI_RPB11,
        SPI_RPB12,
        SPI_RPB13,
        SPI_RPB14,
        SPI_RPB15,
        SPI_NO_PIN
} SPI_PinMap_t;

typedef enum { 
    SPI_CLK_LO = 0, 
    SPI_CLK_HI = 1 
} SPI_Clock_t;

typedef enum { 
    SPI_SECOND_EDGE = 0,
    SPI_FIRST_EDGE = 1, 
} SPI_ActiveEdge_t;

typedef enum { 
    SPI_SMP_MID = 0,
    SPI_SMP_END = 1 
} SPI_SamplePhase_t;

typedef enum { 
    SPI_8BIT = 0, 
    SPI_16BIT = 1,
    SPI_32BIT = 2
} SPI_XferWidth_t;

bool SPISetup_BasicConfig(SPI_Module_t WhichModule);

bool SPISetup_SetFollower(SPI_Module_t WhichModule);
bool SPISetup_SetLeader(SPI_Module_t WhichModule, SPI_SamplePhase_t WhichPhase);

bool SPISetup_SetBitTime(SPI_Module_t WhichModule, uint32_t SPI_ClkPeriodIn_ns);

bool SPISetup_MapSSInput(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin);
bool SPISetup_MapSSOutput(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin);

bool SPISetup_MapSDInput(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin);
bool SPISetup_MapSDOutput(SPI_Module_t WhichModule, SPI_PinMap_t WhichPin);

bool SPISetup_SetClockIdleState(SPI_Module_t WhichModule, SPI_Clock_t WhichState);

bool SPISetup_SetActiveEdge(SPI_Module_t WhichModule, SPI_ActiveEdge_t WhichEdge);

bool SPISetup_SetXferWidth(SPI_Module_t WhichModule, SPI_XferWidth_t DataWidth);

bool SPISetEnhancedBuffer(SPI_Module_t WhichModule, bool IsEnhanced);

bool SPISetup_DisableSPI(SPI_Module_t WhichModule);
bool SPISetup_EnableSPI(SPI_Module_t WhichModule);

void SPIOperate_SPI1_Send8(uint8_t TheData);
void SPIOperate_SPI1_Send16( uint16_t TheData);
void SPIOperate_SPI1_Send32(uint32_t TheData);

void SPIOperate_SPI1_Send8Wait(uint8_t TheData);
void SPIOperate_SPI1_Send16Wait( uint16_t TheData);
void SPIOperate_SPI1_Send32Wait(uint32_t TheData);

uint32_t SPIOperate_ReadData(SPI_Module_t WhichModule);

bool SPIOperate_HasSS1_Risen(void);
bool SPIOperate_HasSS2_Risen(void);

void InitSPI(void);

#endif //PIC32_SPI_HAL defined
