/******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      app_hw.c
 *
 *  DESCRIPTION
 *      This file implements the abstraction for temperature sensor hardware
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/
#include <pio.h>
#include <timer.h>
#include <sys_events.h>
#ifndef CSR101x_A05
#include <mw_pio.h>
#endif
/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "user_config.h"
#include "app_hw.h"
#ifdef TEMPERATURE_SENSOR_STTS751
#include "stts751_temperature_sensor.h"
#endif /* TEMPERATURE_SENSOR_STTS751 */
#include "app_debug.h"
#include "iot_hw.h"
#include "app_mesh_model_handler.h"
#include "app_util.h"

/*============================================================================*
 *  Private Definitions
 *============================================================================*/
/* Time for which the button bounce is expected */
#define BUTTON_DEBOUNCE_TIME                (20 * MILLISECOND)

/* Timer to check if button was pressed for 1 second */
#define BUTTON_ONE_SEC_PRESS_TIME           (1 * SECOND)

/* Button and switch states */
#define KEY_PRESSED                         (TRUE)
#define KEY_RELEASED                        (FALSE)

/*============================================================================*
 *  Private Data
 *============================================================================*/
/* Temperature sensor read delay after issuing read command. */
#ifdef TEMPERATURE_SENSOR_STTS751
#define TEMP_READ_DELAY         (MAX_CONVERSION_TIME * MILLISECOND)
#endif /* TEMPERATURE_SENSOR_STTS751 */

/* Event handler to be called after temperature is read from sensor. */
static TEMPSENSOR_EVENT_HANDLER_T eventHandler;

/* Timer ID for temperature read delay. */
static timer_id read_delay_tid = TIMER_INVALID;

#ifdef ENABLE_TEMPERATURE_CONTROLLER
/* Temperature Controller Button Debounce Timer ID. */
static timer_id oneSecTimerId = TIMER_INVALID;
#ifndef DEBUG_ENABLE
/* Toggle switch de-bounce timer id */
static timer_id debounce_tid = TIMER_INVALID;

/* Switch Button States. */
static bool     incButtonState = KEY_RELEASED;
static bool     decButtonState = KEY_RELEASED;

#ifndef CSR101x_A05
static pio_mask_t  pio_sw3, pio_sw2;
#endif

#endif /* DEBUG_ENABLE */
#endif /* ENABLE_TEMPERATURE_CONTROLLER */


/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/
/*----------------------------------------------------------------------------*
 *  NAME
 *      tempSensorReadyToRead
 *
 *  DESCRIPTION
 *      This function is called after a duration of temperature read delay,
 *      once a read is initiated.
 *
 *  RETURNS
 *      TRUE if initialization is sucessful.
 *
 *----------------------------------------------------------------------------*/
#ifdef TEMPERATURE_SENSOR_STTS751
static void tempSensorReadyToRead(timer_id tid)
{
    int16 temp;

    if (tid == read_delay_tid)
    {
        read_delay_tid = TIMER_INVALID;

        /* Read the temperature. */
        STTS751_ReadTemperature(&temp);
        if (temp != INVALID_TEMPERATURE)
        {
            /* Convert temperature in to 1/32 degree Centigrade units */
            temp = (temp << 1);

            temp += CELSIUS_TO_KELVIN_FACTOR;
        }

        /* Report the temperature read. */
        eventHandler(temp);
    }
}
#endif /* TEMPERATURE_SENSOR_STTS751 */

#ifdef ENABLE_TEMPERATURE_CONTROLLER
#ifdef DEBUG_ENABLE
/*----------------------------------------------------------------------------*
 *  NAME
 *      desiredTempChangeTimerHandler
 *
 *  DESCRIPTION
 *      This function handles the case when desired temperature is changed 
 *      through uart.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void desiredTempChangeTimerHandler(timer_id tid)
{
    if (tid == oneSecTimerId)
    {
        oneSecTimerId = TIMER_INVALID;

        /* Check for desired temp change and start transmission */
        HandleDesiredTempChange();
    }
}

/*----------------------------------------------------------------------------*
 * NAME
 *    UartDataRxCallback
 *
 * DESCRIPTION
 *     This callback is issued when data is received over UART. Application
 *     may ignore the data, if not required. For more information refer to
 *     the API documentation for the type "uart_data_out_fn"
 *
 * RETURNS
 *     The number of words processed, return data_count if all of the received
 *     data had been processed (or if application don't care about the data)
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR101x_A05
extern uint16 UartDataRxCallback (void* p_data, uint16 data_count,
                                  uint16* p_num_additional_words )
#else
extern uint16 UartDataRxCallback (uint16* p_data, uint16 data_count)
#endif
{
    bool change = FALSE;
    uint8 *byte = (uint8 *)p_data;

#ifdef CSR101x_A05
    /* Application needs 1 additional data to be received */
    *p_num_additional_words = 1;
#endif

    /* If device is not associated, return. */
    if (!IsSensorConfigured())
    {
        return data_count;
    }

    switch(byte[0])
    {
        case '+':
        {
            change = ModifyDesiredTemp(increment_temp);
        }
        break;

        case '-':
        {
            change = ModifyDesiredTemp(decrement_temp);
        }
        break;

        default:
        break;
    }

    if (change)
    {
        TimerDelete(oneSecTimerId);
        oneSecTimerId = TimerCreate(BUTTON_ONE_SEC_PRESS_TIME, TRUE,
                                    desiredTempChangeTimerHandler);
    }

    return data_count;
}

#else /* DEBUG_ENABLE */
/*----------------------------------------------------------------------------*
 *  NAME
 *      handleButtonDebounce
 *
 *  DESCRIPTION
 *      This function handles De-bounce Timer Events.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleButtonDebounce(timer_id tid)
{
    bool startOneSecTimer = FALSE;
    bool update_nvm = FALSE;

    if( tid == debounce_tid)
    {
        debounce_tid = TIMER_INVALID;

#ifdef CSR101x_A05
        /* Enable PIO Events again */
        PioSetEventMask(BUTTONS_BIT_MASK, pio_event_mode_both);
#else
        PioSetEventMultiple(pio_sw2, 
                            pio_event_mode_both | pio_event_mode_wake_both);
        PioSetEventMultiple(pio_sw3, 
                            pio_event_mode_both | pio_event_mode_wake_both);
#endif
        /* If PIO State is same as before starting de-bounce timer,
         * we have a valid event.
         */
        if ((PioGet(SW3_PIO) == FALSE) && (incButtonState == KEY_RELEASED))
        {
            /* Set State and increment level */
            incButtonState = KEY_PRESSED;

            /* Indicate the desired temp had been modified */
            startOneSecTimer= ModifyDesiredTemp(increment_temp);
        }
        else if ((PioGet(SW3_PIO) == TRUE) && (incButtonState == KEY_PRESSED))
        {
            /* Set state to KEY RELEASE */
            incButtonState = KEY_RELEASED;
            update_nvm = TRUE;
        }

        if ((PioGet(SW2_PIO) == FALSE) && (decButtonState == KEY_RELEASED))
        {
            /* Set State and decrement level */
            decButtonState = KEY_PRESSED;

            /* Indicate the desired temp had been modified */
            startOneSecTimer = ModifyDesiredTemp(decrement_temp);
        }
        else if ((PioGet(SW2_PIO) == TRUE) && (decButtonState == KEY_PRESSED))
        {
            /* Set state to KEY RELEASE */
            decButtonState = KEY_RELEASED;
            update_nvm = TRUE;
        }

        /* Create One Second Timer when flag is set */
        if (startOneSecTimer)
        {
            /* Check for desired temp change and start transmission */
            HandleDesiredTempChange();

            /* Start 1 second timer */
            oneSecTimerId = TimerCreate(BUTTON_ONE_SEC_PRESS_TIME, TRUE,
                                                    handleButtonDebounce);
        }
    }
    else if (tid == oneSecTimerId)
    {
        oneSecTimerId = TIMER_INVALID;

        /* Key has been held Pressed for a second now */
        if ((PioGet(SW3_PIO) == FALSE) && (incButtonState == KEY_PRESSED))
        {
            /* Indicate the desired temp had been modified */
            ModifyDesiredTemp(increment_temp);

            /* Check for desired temp change and start transmission */
            HandleDesiredTempChange();

            oneSecTimerId = TimerCreate(BUTTON_ONE_SEC_PRESS_TIME, TRUE,
                                                    handleButtonDebounce);
        }

        /* Key has been held Pressed for a second now */
        if ((PioGet(SW2_PIO) == FALSE) && (decButtonState == KEY_PRESSED))
        {
            /* Indicate the desired temp had been modified */
            ModifyDesiredTemp(decrement_temp);

            /* Check for desired temp change and start transmission */
            HandleDesiredTempChange();

            oneSecTimerId = TimerCreate(BUTTON_ONE_SEC_PRESS_TIME, TRUE,
                                                    handleButtonDebounce);
        }
    }
}
#endif /* DEBUG_ENABLE */
#endif /* ENABLE_TEMPERATURE_CONTROLLER */

/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/

#ifdef ENABLE_TEMPERATURE_CONTROLLER
#ifndef DEBUG_ENABLE
#ifdef CSR101x_A05
/*-----------------------------------------------------------------------------*
 *  NAME
 *      HandlePIOChangedEvent
 *
 *  DESCRIPTION
 *      This function handles the PIO Events.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void HandlePIOChangedEvent(pio_changed_data *data)
{
    bool start_timer = FALSE;
    uint32 pio_changed = data->pio_cause & (SW2_MASK|SW3_MASK);

    if (pio_changed & BUTTONS_BIT_MASK)
    {
        /* Check if PIO Changed from previous state */
        if (((PioGet(SW3_PIO) == FALSE) && (incButtonState == KEY_RELEASED)) ||
            ((PioGet(SW3_PIO) == TRUE) && (incButtonState == KEY_PRESSED)))
        {
            start_timer = TRUE;
        }

        if (((PioGet(SW2_PIO) == FALSE) && (decButtonState == KEY_RELEASED)) ||
            ((PioGet(SW2_PIO) == TRUE) && (decButtonState == KEY_PRESSED)))
        {
            start_timer = TRUE;
        }
    }

    if (start_timer)
    {
        /* Disable further Events */
        PioSetEventMask(BUTTONS_BIT_MASK, pio_event_mode_disable);

        /* Start a timer for de-bounce and delete one second timer
         * as we received a new event.
         */
        TimerDelete(oneSecTimerId);
        oneSecTimerId = TIMER_INVALID;
        TimerDelete(debounce_tid);
        debounce_tid =
                TimerCreate(BUTTON_DEBOUNCE_TIME, TRUE, handleButtonDebounce);
    }
}
#else  /* CSR101x_A05 */
/*-----------------------------------------------------------------------------*
 *  NAME
 *      HandlePIOChangedEvent
 *
 *  DESCRIPTION
 *      This function handles the PIO Events.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void HandlePIOChangedEvent(pio_changed_data *data)
{
    pio_changed_data *event_data;
    bool start_timer = FALSE;
    uint16      sw3_index, sw2_index;

    /* The PIO changed data is defined by struct pio_changed_data */
    event_data = (pio_changed_data *)data;

    /* Get the IOT button pio and mask */
    GetIOTSwitchMask(SW3_PIO, &pio_sw3, &sw3_index);
    GetIOTSwitchMask(SW2_PIO, &pio_sw2, &sw2_index);

    /* If the PIO event comes from the button */
    if((event_data->pio_cause.mask[sw3_index] & pio_sw3.mask[sw3_index]))
    {
        /* Was this a button pressed report */
        if(!(event_data->pio_state.mask[sw3_index] & pio_sw3.mask[sw3_index]))
        {
            /* The button was pressed */
            if(incButtonState == KEY_RELEASED)
            {
                start_timer = TRUE;
            }
        }
        else
        {
            /* The button was released */
            if(incButtonState == KEY_PRESSED)
            {
                start_timer = TRUE;
            }
        }
    }

    /* If the PIO event comes from the button */
    if((event_data->pio_cause.mask[sw2_index] & pio_sw2.mask[sw2_index]))
    {
        /* Was this a button pressed report */
        if(!(event_data->pio_state.mask[sw2_index] & pio_sw2.mask[sw2_index]))
        {
            /* The button was pressed */
            if(decButtonState == KEY_RELEASED)
            {
                start_timer = TRUE;
            }
        }
        else
        {
            /* The button was released */
            if(decButtonState == KEY_PRESSED)
            {
                start_timer = TRUE;
            }
        }
    }

    if (start_timer)
    {
        /* Disable further Events */
        PioSetEventMultiple(pio_sw2, pio_event_mode_disable);
        PioSetEventMultiple(pio_sw3, pio_event_mode_disable);

        /* Start a timer for de-bounce and delete one second timer
         * as we received a new event.
         */
        TimerDelete(oneSecTimerId);
        oneSecTimerId = TIMER_INVALID;
        TimerDelete(debounce_tid);
        debounce_tid =
                TimerCreate(BUTTON_DEBOUNCE_TIME, TRUE, handleButtonDebounce);
    }
}
#endif /* CSR101x_A05 */
#endif /* DEBUG_ENABLE */
#endif /* ENABLE_TEMPERATURE_CONTROLLER */

/*----------------------------------------------------------------------------*
 *  NAME
 *      TempSensorHardwareInit
 *
 *  DESCRIPTION
 *      This function initialises the temperature sensor hardware.
 *
 *  RETURNS
 *      TRUE if initialization is sucessful.
 *
 *----------------------------------------------------------------------------*/
extern bool TempSensorHardwareInit(TEMPSENSOR_EVENT_HANDLER_T handler)
{
    bool status = FALSE;

    read_delay_tid = TIMER_INVALID;

    if (NULL != handler)
    {
        eventHandler = handler;
#ifdef TEMPERATURE_SENSOR_STTS751
        status = STTS751_Init();
#endif /* TEMPERATURE_SENSOR_STTS751 */
    }

    return status;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      TempSensorRead
 *
 *  DESCRIPTION
 *      This function sends a temperature read command to the sensor.
 *      Temperature will be reported in the registered event handler.
 *
 *  RETURNS
 *      TRUE command is sent.
 *
 *----------------------------------------------------------------------------*/
extern bool TempSensorRead(void)
{
    bool status = FALSE;

    /* Return FALSE if already a read is in progress. */
    if (TIMER_INVALID == read_delay_tid)
    {
#ifdef TEMPERATURE_SENSOR_STTS751
        status = STTS751_InitiateOneShotRead();
#endif /* TEMPERATURE_SENSOR_STTS751 */
    }

    /* Command is issued without failure, start the delay timer. */
    if (status)
    {
#ifdef TEMPERATURE_SENSOR_STTS751
        read_delay_tid = TimerCreate(TEMP_READ_DELAY, TRUE,
                                     tempSensorReadyToRead);
#endif /* TEMPERATURE_SENSOR_STTS751 */
    }

    return status;
}

