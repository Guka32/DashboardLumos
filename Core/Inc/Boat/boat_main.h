#include <stdbool.h>
#include "stm32f4xx_hal.h"

enum BoatState {
	BoatState_Autonomous = 0,
	BoatState_Teleoperated = 1,
	BoatState_Disabled = 2
};

//Function only runs once, all tasks should be added here
void createTasks_boat();

/**
 * Tasks
 */
void mainTask_boat(void *params);

void boat_arming_sequence();

void boat_autonomous_loop();
void boat_teleoperated_loop();
void boat_disabled_loop();
