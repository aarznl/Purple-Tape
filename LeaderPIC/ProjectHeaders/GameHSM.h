/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef GameHSM_H
#define GameHSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum {
    DriveToDispenser,
    DispenserToBucket,
    TapeToBeacon,
    BeaconToTape,
    TapeToDispenser,
    GameOver
} GameState_t;


// Public Function Prototypes

ES_Event_t RunGameSM( ES_Event_t CurrentEvent );
void StartGameSM ( ES_Event_t CurrentEvent );
GameState_t QueryGameSM ( void );

#endif /*SHMTemplate_H */

