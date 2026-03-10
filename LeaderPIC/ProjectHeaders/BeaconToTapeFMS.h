/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef BeaconToTapeFSM_H
#define BeaconToTapeFSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum {
    TurningLeft_BT,
    DriveFwd_BT,
    TurningRight_BT,
//    Reverse,
//    AlignWithTape,
    ActuateArm_BT,
    DispenseBalls_BT
} BeaconToTapeState_t;


// Public Function Prototypes

ES_Event_t RunBeaconToTapeFSM( ES_Event_t CurrentEvent );
void StartBeaconToTapeFSM ( ES_Event_t CurrentEvent );
BeaconToTapeState_t QueryBeaconToTapeFSM ( void );

#endif /*SHMTemplate_H */

