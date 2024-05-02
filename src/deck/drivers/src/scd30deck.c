
#define DEBUG_MODULE "scd30Deck"
// All includes
#include "debug.h"
#include "i2cdev.h"
#include "deck.h"
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "system.h"
#include "cf_math.h"

// Addresses, constants etc for SCD30
#define SCD30I2CAddr 0x61  // for ADDR (or CS) pin low

#define SCD30_TASK_STACKSIZE    (2 * configMINIMAL_STACK_SIZE)
#define SCD30_TASK_PRI 3
#define SCD30_TASK_NAME "SCD30"

// Misc global variables
static bool isInit;
// uint16_t raw_temp;
void scd30Task(void* arg);

float co2Concentration;
float temperature;
float humidity;

// Deck driver init function
static void scd30Init()
{
  if (isInit)
    return;

  DEBUG_PRINT("Initializing SCD30...\n");
  i2cdevInit(I2C1_DEV);

  xTaskCreate(scd30Task, SCD30_TASK_NAME, SCD30_TASK_STACKSIZE, NULL, SCD30_TASK_PRI, NULL);

  isInit = true;
  DEBUG_PRINT("SCD30 initialization complete!\n");
}

// Deck driver test function
static bool scd30Test()
{
  DEBUG_PRINT("SCD30Deck test\n");
  return true;
}

void printRawBuffer(uint8_t* buffer, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        DEBUG_PRINT("0x%02X ", buffer[i]);
        if ((i + 1) % 3 == 0) { // 3바이트마다 줄바꿈으로 가독성 향상
            DEBUG_PRINT("\n");
        }
    }
    DEBUG_PRINT("\n"); // 마지막에 줄바꿈 추가
}


#define NO_ERROR 0
#define ERROR_BUFFER_TOO_SHORT 1



uint16_t getMeasurementResults(const uint8_t* buffer, size_t bufferSize, float* co2Concentration, float* temperature, float* humidity) {
    if (bufferSize < 18) { // 필요한 최소한의 바이트 수 확인
        return ERROR_BUFFER_TOO_SHORT;
    }
    
    union {
        uint32_t uInt32Data;
        float floatData;
    } convert[3]={0};

    // CO2 농도 추출
    convert[0].uInt32Data = (uint32_t)buffer[0] << 24 ;
    convert[0].uInt32Data |=  (uint32_t)buffer[1] << 16 ; 
    convert[0].uInt32Data |=  (uint32_t)buffer[3] << 8 ;
    convert[0].uInt32Data |=   (uint32_t)buffer[4];
    *co2Concentration = convert[0].floatData;
    
    // 온도 추출
    convert[1].uInt32Data = (uint32_t)buffer[6] << 24 ;
    convert[1].uInt32Data |=  (uint32_t)buffer[7] << 16 ; 
    convert[1].uInt32Data |=  (uint32_t)buffer[9] << 8 ;
    convert[1].uInt32Data |=   (uint32_t)buffer[10];
    *temperature = convert[1].floatData;
    
    // 습도 추출
    convert[2].uInt32Data = (uint32_t)buffer[12] << 24 ;
    convert[2].uInt32Data |=  (uint32_t)buffer[13] << 16 ; 
    convert[2].uInt32Data |=  (uint32_t)buffer[15] << 8 ;
    convert[2].uInt32Data |=   (uint32_t)buffer[16];
    *humidity = convert[2].floatData;

    return NO_ERROR;
}

uint16_t getReadyResults(const uint8_t* buffer, size_t bufferSize, uint16_t* dataReady){
    *dataReady = (uint16_t)buffer[0] << 8;
    *dataReady |= (uint16_t)buffer[1];

    return NO_ERROR;
}

void scd30Task(void* arg)
{
    uint16_t dataReady = 0;
    uint8_t DatareadBuffer[18]; // CO2, 온도, 습도 값을 읽기 위한 버퍼
    uint8_t DatareadyBuffer[3];
    bool readResult;
    bool writeResult;

    systemWaitStart();
    TickType_t xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount();
    uint8_t startMeasurementCmd[2] = {0x00, 0x10}; // 측정 시작 명령
    writeResult = i2cdevWrite(I2C1_DEV, (uint8_t)SCD30I2CAddr, 2, startMeasurementCmd);
    if (!writeResult) {
        DEBUG_PRINT("Failed to start measurement\n");
    }

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, M2T(100));

        uint8_t DataReadyCmd[2] = {0x02, 0x02}; // 측정 준비 확인 명령
        writeResult = i2cdevWrite(I2C1_DEV, (uint8_t)SCD30I2CAddr, 2, DataReadyCmd);
        if (!writeResult) {
            DEBUG_PRINT("Failed to measurement\n");
        }

        readResult = i2cdevRead(I2C1_DEV, (uint8_t)SCD30I2CAddr, 3, DatareadyBuffer);
        if (readResult) {
            getReadyResults(DatareadyBuffer, 3, &dataReady);
            DEBUG_PRINT("Data reday : %d\n", dataReady);
        } else {
            DEBUG_PRINT("Failed to read data ready from SCD30\n");
        }

        if(dataReady == 1){
            uint8_t MeasurementCmd[2] = {0x03, 0x00}; // 측정 확인 명령
            writeResult = i2cdevWrite(I2C1_DEV, (uint8_t)SCD30I2CAddr, 2, MeasurementCmd);
            if (!writeResult) {
                DEBUG_PRINT("Failed to measurement\n");
            }

            // 데이터 읽기 시도
            readResult = i2cdevRead(I2C1_DEV, (uint8_t)SCD30I2CAddr, 18, DatareadBuffer);
            if (readResult) {
                // for debug
                // printRawBuffer(readBuffer, 18);
                getMeasurementResults(DatareadBuffer, 18, &co2Concentration, &temperature, &humidity);
                DEBUG_PRINT("Temperature : %f\n", temperature);
                DEBUG_PRINT("CO2 : %f\n", co2Concentration);
                DEBUG_PRINT("Humidity : %f\n", humidity);
            } else {
                DEBUG_PRINT("Failed to read data from SCD30\n");
            }
        }
    }
}


static const DeckDriver scd30Driver = {
  .name = "scd30Deck",
  .usedPeriph = DECK_USING_I2C,
  .init = scd30Init,
  .test = scd30Test,
  
};

DECK_DRIVER(scd30Driver);
LOG_GROUP_START(SCD30)
LOG_ADD(LOG_FLOAT, co2Concentration, &co2Concentration)
LOG_ADD(LOG_FLOAT, temperature, &temperature)
LOG_ADD(LOG_FLOAT, humidity, &humidity)
LOG_GROUP_STOP(SCD30)