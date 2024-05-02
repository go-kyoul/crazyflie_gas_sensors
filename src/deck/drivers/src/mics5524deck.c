
#define DEBUG_MODULE "mics5524Deck"
// All includes
#include "debug.h"
#include "deck.h"
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "system.h"
#include "cf_math.h"

// #define MICS5524_PIN DECK_GPIO_TX2
#define MICS5524_TASK_STACKSIZE    (2 * configMINIMAL_STACK_SIZE)
#define MICS5524_TASK_PRI 3
#define MICS5524_TASK_NAME "MICS5524"

// Misc global variables
static bool isInit;
// uint16_t raw_temp;
void mics5524Task(void* arg);

float concentration;


// Deck driver init function
static void mics5524Init()
{
  if (isInit)
    return;

  DEBUG_PRINT("Initializing MICS5524...\n");
  i2cdevInit(I2C1_DEV);

  xTaskCreate(mics5524Task, MICS5524_TASK_NAME, MICS5524_TASK_STACKSIZE, NULL, MICS5524_TASK_PRI, NULL);

  isInit = true;
  DEBUG_PRINT("MICS5524 initialization complete!\n");
}

// Deck driver test function
static bool mics5524Test()
{
  DEBUG_PRINT("MICS5524Deck test\n");
  return true;
}


void mics5524Task(void* arg)
{
    systemWaitStart();
    TickType_t xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        vTaskDelayUntil(&xLastWakeTime, M2T(100));

        concentration = analogReadVoltage(DECK_GPIO_TX2);

        DEBUG_PRINT("data ");
    }
}



static const DeckDriver mics5524Driver = {
  .name = "mics5524Deck",
  .init = mics5524Init,
  .test = mics5524Test,
  
};

DECK_DRIVER(mics5524Driver);
LOG_GROUP_START(MICS5524)
LOG_ADD(LOG_FLOAT, concentration, &concentration)
LOG_GROUP_STOP(MICS5524)