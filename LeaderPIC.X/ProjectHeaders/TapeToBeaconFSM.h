/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef TapeToBeaconFSM_H
#define TapeToBeaconFSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum
{
    TurningLeft,
    DriveFwd,
    TurningRight,
    ActuateArm_TB,
    Dispensing,

} TapeToBeaconState_t;


// Public Function Prototypes

ES_Event_t RunTapeToBeaconFSM( ES_Event_t CurrentEvent );
void StartTapeToBeaconFSM ( ES_Event_t CurrentEvent );
TapeToBeaconState_t QueryTapeToBeaconFSM ( void );

#endif /*SHMTemplate_H */

