/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts

 ****************************************************************************/

#ifndef TopLevelHSM_H
#define TopLevelHSM_H

// State definitions for use with the query function
typedef enum {
    IDLE_STATE,
    ORIENTATION_STATE,
    GAME_STATE
} MasterState_t;

// Public Function Prototypes

ES_Event_t RunMasterSM( ES_Event_t CurrentEvent );
void StartMasterSM ( ES_Event_t CurrentEvent );
bool PostMasterSM( ES_Event_t ThisEvent );
bool InitMasterSM ( uint8_t Priority );

#endif /*TopHSMTemplate_H */

