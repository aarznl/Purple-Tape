/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef DispenserToBucketFSM_H
#define DispenserToBucketFSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum
{
    Bwd_DB,
    Bwd_To_Bucket,
    ActuateArm_DB,
    DispenseBalls,

} DispenserState_t;


// Public Function Prototypes

ES_Event_t RunDispenserToBucketFSM( ES_Event_t CurrentEvent );
void StartDispenserToBucketFSM ( ES_Event_t CurrentEvent );
DispenserState_t QueryDispenserToBucketSM ( void );

#endif /*SHMTemplate_H */

