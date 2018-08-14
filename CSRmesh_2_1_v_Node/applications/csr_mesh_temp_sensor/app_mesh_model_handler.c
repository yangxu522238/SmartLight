/******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      app_mesh_handler.c
 *
 *
 ******************************************************************************/
 /*============================================================================*
 *  SDK Header Files
 *============================================================================*/
#include <main.h>
#ifdef CSR101x_A05
#include <ls_app_if.h>
#include <config_store.h>
#endif
#include <nvm.h>
#include <timer.h>
/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "user_config.h"
#include "nvm_access.h"
#include "app_debug.h"
#include "main_app.h"
#include "app_hw.h"
#include "sensor_client.h"
#include "sensor_model_handler.h"
#include "battery_model_handler.h"
#include "attention_model_handler.h"
#include "time_model_handler.h"
#include "action_model_handler.h"
#include "actuator_model_handler.h"
#include "firmware_model_handler.h"
#include "app_mesh_handler.h"
#include "app_mesh_model_handler.h"
#include "app_util.h"

/*============================================================================*
 *  Private Definitions
 *============================================================================*/
/* Increment/Decrement Step for Button Press. */
#define STEP_SIZE_PER_BUTTON_PRESS          (32)

/* Macro to convert Celsius to 1/32 kelvin units. */
#define CELSIUS_TO_BY32_KELVIN(x)          (((x)*32) + CELSIUS_TO_KELVIN_FACTOR)

/* Max. Temperature. */
#define MAX_DESIRED_TEMPERATURE             (CELSIUS_TO_BY32_KELVIN(40))

/* Min. Temperature. */
#define MIN_DESIRED_TEMPERATURE             (CELSIUS_TO_BY32_KELVIN(-5))

/* Absolute Difference of two numbers. */
#ifndef ABS_DIFF
#define ABS_DIFF(x,y) (((x) > (y))?((x) - (y)):((y) - (x)))
#endif

/* Max sensor type supp in this app sensor_type_desired_air_temperature = 3*/
#define SENSOR_TYPE_SUPPORTED_MAX          (sensor_type_desired_air_temperature)

/* Max transmit msg density */
#define MAX_TRANSMIT_MSG_DENSITY            (6)

#ifdef ENABLE_ACK_MODE
/* The maximum number of heater devices that in grp */
#define MAX_HEATERS_IN_GROUP                (5)

/* The heater will be removed from the added list if it does not respond to 
 * maximim no response count times.
 */
#define MAX_NO_RESPONSE_COUNT               (5)
#endif /* ENABLE_ACK_MODE */

typedef struct
{
    sensor_type_t type;
    uint16        *value;
    uint8         repeat_interval;
} SENSOR_DATA_T;

#ifdef ENABLE_ACK_MODE
typedef struct 
{
    uint16 dev_id;
    bool ack_recvd;
    uint16 no_response_count;
}HEATER_INFO_T;
#endif /* ENABLE_ACK_MODE */

/*============================================================================*
 *  Private Data
 *============================================================================*/

#ifdef ENABLE_ATTENTION_MODEL
static ATTENTION_HANDLER_DATA_T         g_attention_handler_data;
#endif /* ENABLE_ATTENTION_MODEL */

#ifdef ENABLE_BATTERY_MODEL
static BATTERY_HANDLER_DATA_T           g_battery_handler_data;
#endif /* ENABLE_BATTERY_MODEL */

#ifdef ENABLE_SENSOR_MODEL
static SENSOR_HANDLER_DATA_T            g_sensor_handler_data;
#endif /* ENABLE_SENSOR_MODEL */

#ifdef ENABLE_ACTUATOR_MODEL
static ACTUATOR_HANDLER_DATA_T          g_actuator_handler_data;
#endif /* ENABLE_ACTUATOR_MODEL */

#ifdef ENABLE_TIME_MODEL
static TIME_HANDLER_DATA_T              g_time_handler_data;
#endif /* ENABLE_TIME_MODEL */

#ifdef ENABLE_FIRMWARE_MODEL
static FIRMWARE_HANDLER_DATA_T          g_fw_handler_data;
#endif /* ENABLE_FIRMWARE_MODEL */

/* Sensor Model Data */
static SENSOR_DATA_T                    sensor_data[NUM_SENSORS_SUPPORTED];

/* Temperature Sensor Sample Timer ID. */
static timer_id                         tempsensor_sample_tid = TIMER_INVALID;

/* Retransmit Timer ID. */
static timer_id                         retransmit_tid = TIMER_INVALID;

/* Repeat Interval Timer ID. */
static timer_id                         repeat_interval_tid = TIMER_INVALID;

/* Write Value Msg Retransmit counter */
static uint16                           write_val_retransmit_count = 0;

/* Temperature Value in 1/32 kelvin units. */
static SENSOR_FORMAT_TEMPERATURE_T      current_air_temp;

/* Last Broadcast temperature in 1/32 kelvin units. */
static SENSOR_FORMAT_TEMPERATURE_T      last_bcast_air_temp;

/* Temperature Controller's Current Desired Air Temperature. */
static SENSOR_FORMAT_TEMPERATURE_T      current_desired_air_temp;

/* Temperature Controller's Last Broadcast Desired Air Temperature. */
static SENSOR_FORMAT_TEMPERATURE_T      last_bcast_desired_air_temp;

/* Retransmit interval based on the msg transmit density 
 * These values are calculated based on the number of tx msgs added in queue.
 * TRANSMIT_MSG_DENSITY = 1 -> 90ms + (random 0-12.8ms) * 5 -> 500ms.(Approx)
 * TRANSMIT_MSG_DENSITY = 2 -> 45ms + (random 0-12.8ms) * 11 -> 700ms.(Approx)
 * TRANSMIT_MSG_DENSITY = 3 -> 30ms + (random 0-12.8ms) * 17 -> 800ms.(Approx)
 * TRANSMIT_MSG_DENSITY = 4 -> 22.5ms + (random 0-12.8ms) * 23 -> 900ms.(Approx)
 * TRANSMIT_MSG_DENSITY = 5 -> 20ms + (random 0-12.8ms) * 35 -> 1100ms.(Approx)
 * TRANSMIT_MSG_DENSITY = 6 -> 20ms + (random 0-12.8ms) * 41 -> 1400ms.(Approx)
 * tx msg density 6 used when transmit index is dynamically set when configured
 * to multiple groups.
 */
static uint32 retransmit_interval[MAX_TRANSMIT_MSG_DENSITY]={500 * MILLISECOND,
                                                             700 * MILLISECOND,
                                                             800 * MILLISECOND,
                                                             900 * MILLISECOND,
                                                            1100 * MILLISECOND,
                                                            1400 * MILLISECOND};

/* variable to store the msg transmit density */
static uint8 transmit_msg_density = TRANSMIT_MSG_DENSITY;

#ifdef ENABLE_ACK_MODE
/* Stores the device info of the heaters participating in the group.*/
static HEATER_INFO_T heater_list[MAX_HEATERS_IN_GROUP];
#endif /* ENABLE_ACK_MODE */

/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/
#ifdef ENABLE_SENSOR_MODEL
static void writeTempValue(void);
#endif
static void startRetransmitTimer(void);
static void repeatIntervalTimerHandler(timer_id tid);
static void startTempTransmission(void);

/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/

#ifdef ENABLE_ACK_MODE
/*----------------------------------------------------------------------------*
 *  NAME
 *      resetHeaterList
 *
 *  DESCRIPTION
 *      The function resets the device id and the ack flag of complete db
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void resetHeaterList(void)
{
    uint16 idx;

    for(idx=0; idx < MAX_HEATERS_IN_GROUP; idx++)
    {
        heater_list[idx].dev_id = MESH_BROADCAST_ID;
        heater_list[idx].ack_recvd = FALSE;
        heater_list[idx].no_response_count = 0;
    }
}
/*----------------------------------------------------------------------------*
 *  NAME
 *      resetAckInHeaterList
 *
 *  DESCRIPTION
 *      The function resets the ack flag of complete db with valid dev id
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void resetAckInHeaterList(void)
{
    uint16 idx;

    for(idx=0; idx < MAX_HEATERS_IN_GROUP; idx++)
    {
        if(heater_list[idx].dev_id != MESH_BROADCAST_ID)
        {
            heater_list[idx].ack_recvd = FALSE;
        }
    }
}
#endif /* ENABLE_ACK_MODE */

/*-----------------------------------------------------------------------------*
 *  NAME
 *      getSensorGroupCount
 *
 *  DESCRIPTION
 *      This below function returns the number of groups the sensor is 
 *      configured onto
 *
 *  RETURNS/MODIFIES
 *      The number of groups the sensor model is configured
 *
 *----------------------------------------------------------------------------*/
static uint8 getSensorGroupCount(void)
{
    uint8 index, num_groups=0;

    for(index = 0; index < MAX_MODEL_GROUPS; index++)
    {
        if(sensor_model_groups[index] != 0)
        {
            num_groups++;
        }
    }
    return num_groups;
}

#ifdef ENABLE_SENSOR_MODEL
/*----------------------------------------------------------------------------*
 *  NAME
 *      writeTempValue
 *
 *  DESCRIPTION
 *      This function writes the current and the desired temp values onto the 
 *      groups.
 *
 *  RETURNS
 *      Nothing.
 *
 
*----------------------------------------------------------------------------*/
static void writeTempValue(void)
{
    uint16 index, index1;
    bool ack_reqd = FALSE;
    CSRMESH_SENSOR_WRITE_VALUE_T sensor_values;

#ifdef ENABLE_ACK_MODE 
    ack_reqd = TRUE;
#endif /* ENABLE_ACK_MODE */

    for(index1 = 0; index1 < transmit_msg_density; index1 ++)
    {
        for(index = 0; index < MAX_MODEL_GROUPS; index++)
        {
            if(sensor_model_groups[index] != 0 && 
               current_air_temp != 0 &&
               current_desired_air_temp != 0)
            {
                /* Retransmitting same messages back to back multiple times 
                 * increases the probability of the message being received by 
                 * devices running on low duty cycle scan. 
                 * This can be tuned by setting the TRANSMIT_MESSAGE_DENSITY 
                 */
                /* transmit the pending message to all the groups */
                sensor_values.type = sensor_type_internal_air_temperature;
                sensor_values.value[0] = current_air_temp & 0xFF;
                sensor_values.value[1] = ((current_air_temp >> 8) & 0xFF);
                sensor_values.value_len = 2;
                sensor_values.type2 = sensor_type_desired_air_temperature;
                sensor_values.value2[0] = current_desired_air_temp & 0xFF;
                sensor_values.value2[1] = ((current_desired_air_temp >> 8) & 0xFF);
                sensor_values.value2_len= 2;
                sensor_values.tid = 0;
                SensorWriteValue(0,
                                 sensor_model_groups[index],
                                 AppGetCurrentTTL(),
                                 &sensor_values,
                                 ack_reqd);
            }
        }
    }
}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      retransmitIntervalTimerHandler
 *
 *  DESCRIPTION
 *      This function expires when the next message needs to be transmitted 
 *
 *  RETURNS
 *      Nothing.
 *
 
*----------------------------------------------------------------------------*/
static void retransmitIntervalTimerHandler(timer_id tid)
{
    if (tid == retransmit_tid)
    {
        bool start_timer = TRUE;

        retransmit_tid = TIMER_INVALID;

#ifdef ENABLE_SENSOR_MODEL
        /* transmit the pending message to all the groups*/
        writeTempValue();
#endif
        write_val_retransmit_count --;

#ifdef ENABLE_ACK_MODE 
        bool skip_ack_check = FALSE;
        /* After half of the max retransmissions are over then check whether
         * ack has been received from all the heaters stored and if so then
         * stop sending the packet as we have received acks for all heaters.
         */
        if(write_val_retransmit_count < (NUM_OF_RETRANSMISSIONS/2))
        {
            uint8 idx;
            for(idx=0; idx < MAX_HEATERS_IN_GROUP; idx++)
            {
                 if(heater_list[idx].dev_id != MESH_BROADCAST_ID)
                 {
                    break;
                 }
                 if(idx == (MAX_HEATERS_IN_GROUP-1))
                 {
                    skip_ack_check = TRUE;
                 }
            }
            if(skip_ack_check == FALSE)
            {
                for(idx=0; idx < MAX_HEATERS_IN_GROUP; idx++)
                {
                    if(heater_list[idx].dev_id != MESH_BROADCAST_ID &&
                       heater_list[idx].ack_recvd == FALSE)
                    {
                        break;
                    }
                    if(idx == (MAX_HEATERS_IN_GROUP-1))
                    {
                        start_timer = FALSE;
                        DEBUG_STR(" RECVD ALL ACK'S STOP TIMER : ");
                    }
                }
                /* One or more devices have not acked back increase the no response
                 * count. If the no response count reaches the maximum, remove the
                 * device from the heater list.
                 */
                if(write_val_retransmit_count == 0)
                {
                    for(idx=0; idx < MAX_HEATERS_IN_GROUP; idx++)
                    {
                        if(heater_list[idx].dev_id != MESH_BROADCAST_ID &&
                           heater_list[idx].ack_recvd == FALSE)
                        {
                            heater_list[idx].no_response_count++;
                            if(heater_list[idx].no_response_count >= 
                                                            MAX_NO_RESPONSE_COUNT)
                            {
                                heater_list[idx].dev_id = MESH_BROADCAST_ID;
                                heater_list[idx].no_response_count = 0;
                            }
                        }
                    }
                }
            }
        }
#endif /* ENABLE_ACK_MODE */ 
 
        if(start_timer == TRUE)
        {
            /* start a timer to send the broadcast sensor data */
            startRetransmitTimer();
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      startRetransmitTimer
 *
 *  DESCRIPTION
 *      This function starts the broadcast timer for the current tempertature.
 *
 *  RETURNS
 *      None
 *
 
*----------------------------------------------------------------------------*/
static void startRetransmitTimer(void)
{
    if(write_val_retransmit_count > 0 && getSensorGroupCount() > 0)
    {
        uint8 transmit_index = 
                    (transmit_msg_density * getSensorGroupCount()) -1;

        retransmit_tid=TimerCreate(
                     retransmit_interval[transmit_index],
                     TRUE,
                     retransmitIntervalTimerHandler);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      tempSensorSampleIntervalTimeoutHandler
 *
 *  DESCRIPTION
 *      This function handles the sensor sample interval time-out.
 *
 *  RETURNS
 *      Nothing.
 *
 
*----------------------------------------------------------------------------*/
static void tempSensorSampleIntervalTimeoutHandler(timer_id tid)
{
    if (tid == tempsensor_sample_tid)
    {
        tempsensor_sample_tid = TIMER_INVALID;

        /* Issue a Temperature Sensor Read. */
        TempSensorRead();

        /* Start the timer for next sample. */
        tempsensor_sample_tid = TimerCreate(
                                        (uint32)TEMPERATURE_SAMPLING_INTERVAL, 
                                        TRUE,
                                        tempSensorSampleIntervalTimeoutHandler);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      tempSensorEvent
 *
 *  DESCRIPTION
 *      This function Handles the sensor read complete event. Checks if the new 
 *      temperature is within tolerence from the last broadcast value, otherwise
 *      broadcasts new value.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void tempSensorEvent(int16 temp)
{
    /* Check if the new value is valid. */
    if (temp > 0)
    {
        uint16 abs_diff;
        SENSOR_FORMAT_TEMPERATURE_T cur_temp_returned = temp;

        /* If Desired air temperature is not Initialised, Initialise it based 
         * on current air temperature.
         */
        if (current_desired_air_temp == 0x0000)
        {
            current_desired_air_temp = cur_temp_returned;
        }

        DEBUG_STR(" CURR AIR TEMP read : ");
        PrintInDecimal(cur_temp_returned/32);
        DEBUG_STR(" kelvin\r\n");

        /* Find the absolute difference between current and last broadcast
         * temperatures.
         */
        abs_diff = ABS_DIFF((uint16)last_bcast_air_temp, cur_temp_returned);

        /* If it changed beyond the tolerance value, then write the current 
         * temp onto the group.
         */
        if (abs_diff >= TEMPERATURE_CHANGE_TOLERANCE)
        {
            /* Set the present temperature as it is more than the temperature
             * tolerance change. Write the new value to the group as well as
             * reset the retransmit count to max and start the retransmit timer
             * if its not started.
             */
            current_air_temp = cur_temp_returned;

            /* Set last Broadcast Temperature to current temperature. */
            last_bcast_air_temp = current_air_temp;

            startTempTransmission();
        }
    }
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      startRepeatIntervalTimer
 *
 *  DESCRIPTION
 *      Start the repeat interval timer  The function should be called only if
 *      the repeat interval on the desired or the current index is non zero.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void startRepeatIntervalTimer (void)
{
    /* The app takes the minimum value of the repeat interval from current and
     * desired repeat intervals.
     */
    uint8 interval = sensor_data[CURRENT_AIR_TEMP_IDX].repeat_interval;

    if(sensor_data[CURRENT_AIR_TEMP_IDX].repeat_interval == 0 || 
      (sensor_data[DESIRED_AIR_TEMP_IDX].repeat_interval !=0 && 
        sensor_data[DESIRED_AIR_TEMP_IDX].repeat_interval < 
        sensor_data[CURRENT_AIR_TEMP_IDX].repeat_interval))
    {
        interval = sensor_data[DESIRED_AIR_TEMP_IDX].repeat_interval;
    }

    /* As the application transmits the same message for a longer period of time
     * we would consider the repeat interval values below 30 seconds to be 
     * atleast 30 seconds.
     */
    if(interval < 30)
    {
        interval = 30;
    }

    /* Delete the repeat interval timer */
    TimerDelete(repeat_interval_tid);
    repeat_interval_tid = TIMER_INVALID;

    repeat_interval_tid = TimerCreate(((uint32)interval* SECOND),
            TRUE, repeatIntervalTimerHandler);

    /* Start the transmission once here */
    startTempTransmission();
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      repeatIntervalTimerHandler
 *
 *  DESCRIPTION
 *      This function handles repeat interval time-out.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void repeatIntervalTimerHandler(timer_id tid)
{
    if (repeat_interval_tid == tid)
    {
        repeat_interval_tid = TIMER_INVALID;
        startRepeatIntervalTimer();
    }
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      InitialiseSensorData
 *
 *  DESCRIPTION
 *      This function initialises supported sensor data.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void InitialiseSensorData(void)
{
    /* Initialise Temperature Sensor Hardware. */
    if (!TempSensorHardwareInit(tempSensorEvent))
    {
        DEBUG_STR("\r\nFailed to Init temp sensor\r\n");
    }

    /* Initialise Application specific Sensor Model Data.
     * This needs to be done before readPersistentStore.
         */
    sensor_data[CURRENT_AIR_TEMP_IDX].repeat_interval = 
                                        DEFAULT_REPEAT_INTERVAL & 0xFF;
    sensor_data[CURRENT_AIR_TEMP_IDX].type        = 
                                        sensor_type_internal_air_temperature;
    sensor_data[CURRENT_AIR_TEMP_IDX].value       = 
                                        (uint16 *)&current_air_temp;

    sensor_data[DESIRED_AIR_TEMP_IDX].repeat_interval = 
                                                DEFAULT_REPEAT_INTERVAL & 0xFF;
    sensor_data[DESIRED_AIR_TEMP_IDX].type        = 
                                        sensor_type_desired_air_temperature;
    sensor_data[DESIRED_AIR_TEMP_IDX].value       = 
                                        (uint16 *)&current_desired_air_temp;
#ifdef ENABLE_SENSOR_MODEL
    g_sensor_handler_data.current_air_temp = 
                                         (uint16 *)&current_air_temp;
    g_sensor_handler_data.current_desired_air_temp = 
                                         (uint16 *)&current_desired_air_temp;
#endif
#ifdef ENABLE_ACTUATOR_MODEL
    g_actuator_handler_data.current_air_temp = 
                                         (uint16 *)&current_air_temp;
    g_actuator_handler_data.current_desired_air_temp = 
                                         (uint16 *)&current_desired_air_temp;
#endif
    tempsensor_sample_tid = TIMER_INVALID;
    retransmit_tid  = TIMER_INVALID;
    repeat_interval_tid = TIMER_INVALID;

#ifdef ENABLE_ACK_MODE 
    resetHeaterList();
#endif /* ENABLE_ACK_MODE */
}


/*----------------------------------------------------------------------------*
 * NAME
 *      startTempTransmission
 *
 * DESCRIPTION
 *      This function starts the transmission of the temp sensor value.
 *
 * RETURNS
 *      Nothing.
 *
 
*----------------------------------------------------------------------------*/
static void startTempTransmission(void)
{
    transmit_msg_density = TRANSMIT_MSG_DENSITY;

    switch(getSensorGroupCount())
    {
        case 2:
            if(transmit_msg_density > 3)
                transmit_msg_density = 3;
            break;

        case 3:
            if(transmit_msg_density > 2)
                transmit_msg_density = 2;
        break;

        case 4:
            transmit_msg_density = 1;
        break;

        default:
        break;
    }

    write_val_retransmit_count = (NUM_OF_RETRANSMISSIONS/transmit_msg_density);

    DEBUG_STR(" RETRANSMIT_COUNT : ");
    PrintInDecimal(write_val_retransmit_count);
    DEBUG_STR("\r\n");

    DEBUG_STR(" TRANSMIT_MSG_DENSITY : ");
    PrintInDecimal(transmit_msg_density);
    DEBUG_STR("\r\n");

    DEBUG_STR(" WRITE DESIRED TEMP : ");
    PrintInDecimal(current_desired_air_temp/32);
    DEBUG_STR(" kelvin\r\n");

    DEBUG_STR(" WRITE AIR TEMP : ");
    PrintInDecimal(current_air_temp/32);
    DEBUG_STR(" kelvin\r\n");
#ifdef ENABLE_SENSOR_MODEL
    writeTempValue();
#endif
    write_val_retransmit_count --;

    TimerDelete(retransmit_tid);
    retransmit_tid  = TIMER_INVALID;
    startRetransmitTimer();

    WriteSensorDataToNVM(DESIRED_AIR_TEMP_IDX);
#ifdef ENABLE_ACK_MODE 
    resetAckInHeaterList();
#endif /* ENABLE_ACK_MODE */
}

/*============================================================================*
 *  Public Function Implemtations
 *============================================================================*/

/*-----------------------------------------------------------------------------*
 *  NAME
 *      InitializeSupportedModelData
 *
 *  DESCRIPTION
 *      This function initializes the mesh model data used by the application.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
extern void InitializeSupportedModelData(void)
{
#ifdef ENABLE_SENSOR_MODEL
        /* Initialize the sensor model */
        SensorModelDataInit(&g_sensor_handler_data);
#endif /* ENABLE_SENSOR_MODEL */

#ifdef ENABLE_ACTUATOR_MODEL
        /* Initialize the actuator model */
        ActuatorModelDataInit(&g_actuator_handler_data);
#endif /* ENABLE_ACTUATOR_MODEL */

#ifdef ENABLE_FIRMWARE_MODEL
        /* Initialize the firmware model */
        FirmwareModelDataInit(&g_fw_handler_data);

        /* Set Firmware Version */
        g_fw_handler_data.fw_version.majorversion = APP_MAJOR_VERSION;
        g_fw_handler_data.fw_version.minorversion = APP_MINOR_VERSION;
#endif /* ENABLE_FIRMWARE_MODEL */

#ifdef ENABLE_TIME_MODEL
        /* Initialize Time Model */
        TimeModelDataInit(&g_time_handler_data);
#endif /* ENABLE_TIME_MODEL */

#ifdef ENABLE_ACTION_MODEL
        /* Initialize Action Model */
        ActionModelDataInit();
#endif /* ENABLE_ACTION_MODEL */

#ifdef ENABLE_ATTENTION_MODEL
        /* Initialize the attention model */
        AttentionModelDataInit(&g_attention_handler_data);
#endif /* ENABLE_ATTENTION_MODEL */

#ifdef ENABLE_BATTERY_MODEL
        /* Initialize the battery model */
        BatteryModelDataInit(&g_battery_handler_data);
#endif /* ENABLE_BATTERY_MODEL */
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      ReadSensorDataFromNVM
 *
 *  DESCRIPTION
 *      This function reads sensor state data from NVM into state variable.
 *
 *  RETURNS
 *      Nothing.
 *
 
*----------------------------------------------------------------------------*/
extern void ReadSensorDataFromNVM(uint16 idx)
{
    Nvm_Read((uint16*)(sensor_data[idx].value), 
             sizeof(uint16),
             (GET_SENSOR_NVM_OFFSET(idx)));

    Nvm_Read((uint16*)&(sensor_data[idx].repeat_interval), 
             sizeof(uint8),
             (GET_SENSOR_NVM_OFFSET(idx) + sizeof(uint8)));

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      WriteSensorDataToNVM
 *
 *  DESCRIPTION
 *      This function writes sensor state data from state variable into NVM.
 *
 *  RETURNS
 *      Nothing.
 *
 
*----------------------------------------------------------------------------*/
extern void WriteSensorDataToNVM(uint16 idx)
{
    Nvm_Write((uint16*)(sensor_data[idx].value), 
              sizeof(uint16),
              (GET_SENSOR_NVM_OFFSET(idx)));

    Nvm_Write((uint16*)&(sensor_data[idx].repeat_interval), 
              sizeof(uint8),
              (GET_SENSOR_NVM_OFFSET(idx) + sizeof(uint8)));
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      ConfigureSensor
 *
 *  DESCRIPTION
 *      The below function is called when the sensor group is modified.
 *
 *  RETURNS/MODIFIES
 *      None
 *
 *----------------------------------------------------------------------------*/
extern void ConfigureSensor(bool old_config)
{
    /* If sensor was previously not grouped and has been grouped now, then the
     * sensor should move into low duty cycle 
     */
    if(!old_config && IsSensorConfigured())
    {
#ifdef ENABLE_TEMPERATURE_CONTROLLER
        /* Print a message for temperature control. */
        DEBUG_STR("\r\nPress '+'/'-' Increase/Decrease Temp.\r\n");
#endif /* ENABLE_TEMPERATURE_CONTROLLER */

        EnableHighDutyScanMode(FALSE);
        DEBUG_STR("Moving to Low Power Sniff Mode \r\n\r\n");

        if(sensor_data[DESIRED_AIR_TEMP_IDX].repeat_interval !=0 ||
           sensor_data[CURRENT_AIR_TEMP_IDX].repeat_interval !=0)
        {
            startRepeatIntervalTimer();
        }
    }
    else if(old_config && !IsSensorConfigured())
    {
        DEBUG_STR("Moving to active scan Mode \r\n\r\n");
        EnableHighDutyScanMode(TRUE);

        /* Delete the repeat interval timer */
        TimerDelete(repeat_interval_tid);
        repeat_interval_tid = TIMER_INVALID;

        /* Stop the periodic reading of the temp */
        TimerDelete(tempsensor_sample_tid);
        tempsensor_sample_tid = TIMER_INVALID;

        /* Stop the retransmissions if already in progress */
        TimerDelete(retransmit_tid);
        retransmit_tid = TIMER_INVALID;
    }

    /* Grouping has been modified but sensor is still configured. Hence 
     * start temp read and update 
     */
    if(IsSensorConfigured())
    {
        /* Reset last bcast temp to trigger temp update on sensor read*/
        last_bcast_air_temp = 0;

        /* Issue a Read to start sampling timer. */
        TempSensorRead();

        /* Stop the retransmissions if already in progress */
        TimerDelete(retransmit_tid);
        retransmit_tid = TIMER_INVALID;

        /* Delete the temp sensor tid and create a new one */
        TimerDelete(tempsensor_sample_tid);
        tempsensor_sample_tid = TIMER_INVALID;

        /* Start the timer for next sample. */
        tempsensor_sample_tid = TimerCreate(
                                (uint32)TEMPERATURE_SAMPLING_INTERVAL, 
                                TRUE,
                                tempSensorSampleIntervalTimeoutHandler);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      EnableTempReading
 *
 *  DESCRIPTION
 *      This function is called to enable the reading of the temperature sensor.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void EnableTempReading(void)
{
   if(IsSensorConfigured())
   {
        DEBUG_STR("Temp Sensor Configured, Moving to Low Power Mode\r\n");

        EnableHighDutyScanMode(FALSE);

        /* Issue a Read to start sampling timer. */
        TempSensorRead();

        /* Start the timer for next sample. */
        tempsensor_sample_tid = TimerCreate(
                                    (uint32)TEMPERATURE_SAMPLING_INTERVAL, 
                                    TRUE,
                                    tempSensorSampleIntervalTimeoutHandler);

        if(sensor_data[DESIRED_AIR_TEMP_IDX].repeat_interval !=0 ||
           sensor_data[CURRENT_AIR_TEMP_IDX].repeat_interval !=0)
        {
            startRepeatIntervalTimer();
        }
    }
    else
    {
        DEBUG_STR("Temp Sensor not grouped\r\n");
    }
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      GetSensorState
 *
 *  DESCRIPTION
 *      This function returns the state value of the specfic sensor requested.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
extern uint8 GetSensorState(sensor_type_t type)
{
    if(type == sensor_type_desired_air_temperature)
    {
        return sensor_data[DESIRED_AIR_TEMP_IDX].repeat_interval;
    }
    else if(type == sensor_type_internal_air_temperature)
    {
        return sensor_data[CURRENT_AIR_TEMP_IDX].repeat_interval;
    }
    else
    {
        return 0;
    }
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      IsSensorConfigured
 *
 *  DESCRIPTION
 *      This below function returns whether the sensor is configured or not
 *
 *  RETURNS/MODIFIES
 *      TRUE if the sensor has been grouped otherwise returns FALSE
 *
 *----------------------------------------------------------------------------*/
extern bool IsSensorConfigured(void)
{
    uint16 index;

    for(index = 0; index < MAX_MODEL_GROUPS; index++)
    {
        if(sensor_model_groups[index] != 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      ModifyDesiredTemp
 *
 *  DESCRIPTION
 *      This function is used to increase or decrease desired temp from hw 
 *      events.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
extern bool ModifyDesiredTemp(modify_temp_t temp_change)
{
    bool ret_val = FALSE;

    if(IsSensorConfigured())
    {
        if(temp_change == increment_temp)
        {
            if (current_desired_air_temp < 
                    (MAX_DESIRED_TEMPERATURE - STEP_SIZE_PER_BUTTON_PRESS))
            {
                current_desired_air_temp += STEP_SIZE_PER_BUTTON_PRESS;
                ret_val = TRUE;
            }
            else
            {
                current_desired_air_temp = MAX_DESIRED_TEMPERATURE;
            }
        }
        else if(temp_change == decrement_temp)
        {
            if (current_desired_air_temp > 
                    (MIN_DESIRED_TEMPERATURE - STEP_SIZE_PER_BUTTON_PRESS))
            {
                current_desired_air_temp -= STEP_SIZE_PER_BUTTON_PRESS;
                ret_val = TRUE;
            }
            else
            {
                current_desired_air_temp = MIN_DESIRED_TEMPERATURE;
            }
        }
    }
    return ret_val;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HandleDesiredTempChange
 *
 *  DESCRIPTION
 *      This function checks for desired temp change and starts a new 
 *      transmission with the new desired temperature.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
extern void HandleDesiredTempChange()
{
    if(IsSensorConfigured())
    {
        if (current_desired_air_temp != last_bcast_desired_air_temp)
        {
            /* Set last Broadcast Temperature to current temperature. */
            last_bcast_desired_air_temp = current_desired_air_temp;

            /* Send New desired temperature. */
            startTempTransmission();
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppSensorModelHandler
 *
 *  DESCRIPTION
 *      This function is called from the sensor model handler to the application
 *      to indicate a message of interest has been received by the model. The
 *      application can handle the required sensor model messages and take 
 *      relavant actions on the same.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
extern void AppSensorModelHandler(SENSOR_APP_DATA_T sensor_app_data)
{
    switch(sensor_app_data.event_code)
    {
        case CSRMESH_SENSOR_READ_VALUE:
        case CSRMESH_SENSOR_MISSING:
        {
            /* Start the current temperature transmission as it would  */
            startTempTransmission();
        }
        break;

        case CSRMESH_SENSOR_SET_STATE:
        {
            if(sensor_app_data.type == sensor_type_internal_air_temperature)
            {
                sensor_data[CURRENT_AIR_TEMP_IDX].repeat_interval =
                                                sensor_app_data.repeatinterval;
                WriteSensorDataToNVM(CURRENT_AIR_TEMP_IDX); 
            }
            else if(sensor_app_data.type == sensor_type_desired_air_temperature)
            {
                sensor_data[DESIRED_AIR_TEMP_IDX].repeat_interval = 
                                                sensor_app_data.repeatinterval;
                WriteSensorDataToNVM(DESIRED_AIR_TEMP_IDX);
            }

            /* As repeat interval might have changed restart or stop the 
             * timer as per the new interval value.
             */
            if(sensor_data[CURRENT_AIR_TEMP_IDX].repeat_interval != 0 ||
               sensor_data[DESIRED_AIR_TEMP_IDX].repeat_interval != 0)
            {
                startRepeatIntervalTimer();
            }
            else
            {
                /* Delete the repeat interval timer */
                TimerDelete(repeat_interval_tid);
                repeat_interval_tid = TIMER_INVALID;
            }
        }
        break;

        case CSRMESH_SENSOR_VALUE:
        {
#ifdef ENABLE_ACK_MODE
            /* We have received acknowledgement for the write_value sent.
             * update the acknowlege heater device list.
             */
            if(sensor_app_data.recvd_curr_temp == last_bcast_air_temp &&
               sensor_app_data.recvd_desired_temp 
                                            == last_bcast_desired_air_temp)
            {
                uint16 index;
                bool dev_found = FALSE;
                uint16 src_id = sensor_app_data.src_id;

                /* Check whether the heater is present in the list if its 
                * present then update the ack recvd from the heater,
                * otherwise add the new heater onto the list and increase 
                * the count.
                */
                for(index = 0; index < MAX_HEATERS_IN_GROUP; index++)
                {
                    if(heater_list[index].dev_id == src_id)
                    {
                        heater_list[index].ack_recvd = TRUE;
                        dev_found = TRUE;
                        heater_list[index].no_response_count = 0;
                        break;
                    }
                }
                if(dev_found == FALSE)
                {
                    for(index = 0; index < MAX_HEATERS_IN_GROUP; index++)
                    {
                        if(heater_list[index].dev_id == MESH_BROADCAST_ID)
                        {
                            heater_list[index].dev_id = src_id;
                            heater_list[index].ack_recvd = TRUE;
                            heater_list[index].no_response_count = 0;
                            break;
                        }
                    }
                }
            }
#endif /* ENABLE_ACK_MODE */
        }
        break;


        case CSRMESH_SENSOR_WRITE_VALUE:
        case CSRMESH_SENSOR_WRITE_VALUE_NO_ACK:
        {
            if(((sensor_app_data.type == sensor_type_desired_air_temperature ||
                sensor_app_data.type2 == sensor_type_desired_air_temperature ))
                && (sensor_app_data.recvd_desired_temp!=0))
            {
                current_desired_air_temp = 
                                    sensor_app_data.recvd_desired_temp;
                HandleDesiredTempChange();
            }
        }
        break;

        default:
        break;
    }
}

