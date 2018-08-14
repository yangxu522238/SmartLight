/******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      app_hw.h
 *
 *  DESCRIPTION
 *      This file defines a interface to abstract hardware specifics of
 *      temperature sensor.
 *
 *  NOTES
 *
 ******************************************************************************/
#ifndef __APP_HW_H__
#define __APP_HW_H__

#include <sys_events.h>
#include "user_config.h"
#include "iot_hw.h"

/*============================================================================*
 *  Public Definitions
 *============================================================================*/
/* Converting 273.15 to 1/32kelvin results in 8740.8. Add the 0.025 to 
 * compensate loss due to integer division.
 */
#define INTEGER_DIV_LOSS_FACTOR (0.025)

/* Celsius to 1/32 kelvin conversion factor = (273.15 * 32) */
#define CELSIUS_TO_KELVIN_FACTOR \
                             (uint16)((273.15 + INTEGER_DIV_LOSS_FACTOR) * 32)

/* Number of Timers required for temperature sensor. */
#define NUM_TEMP_SENSOR_TIMERS      (1)

#ifdef TEMPERATURE_SENSOR_STTS751
#ifndef CSR101x_A05
#define I2C_SCL_PIO                               (16)
#define I2C_SDA_PIO                               (15)
#endif
#endif /* TEMPERATURE_SENSOR_STTS751 */

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/* Function pointer for Temperature Sensor Event Callback. */
typedef void (*TEMPSENSOR_EVENT_HANDLER_T)(int16 temp);

/* Temperature Sensor Hardware Initialization function. */
extern bool TempSensorHardwareInit(TEMPSENSOR_EVENT_HANDLER_T handler);

/* This function initiates a Read temperature operation.
 * Temperature is reported in the Event Handler registered.
 */
extern bool TempSensorRead(void);

#if defined(ENABLE_TEMPERATURE_CONTROLLER) && !defined(DEBUG_ENABLE)
extern void HandlePIOChangedEvent(pio_changed_data *data);
#endif /* defined(ENABLE_TEMPERATURE_CONTROLLER) && !defined(DEBUG_ENABLE) */

#if defined(DEBUG_ENABLE) && defined(ENABLE_TEMPERATURE_CONTROLLER)
#ifdef CSR101x_A05
extern uint16 UartDataRxCallback (void* p_data, uint16 data_count,
                                  uint16* p_num_additional_words );
#else
extern uint16 UartDataRxCallback (uint16* p_data, uint16 data_count);
#endif
#endif 

#endif /* __APP_HW_H__ */
