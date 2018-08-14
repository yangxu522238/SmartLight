/******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      app_hw.c
 *
 *  DESCRIPTION
 *      This file implements the CSRmesh switch hardware specific functions.
 *
 *  NOTE
 *      Default hardware is always IOT board.
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/
#include <types.h>
#include <pio.h>
#ifndef CSR101x_A05
#include <mw_pio.h>
#endif
#include <timer.h>
/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "user_config.h"
#include "iot_hw.h"
#include "app_hw.h"
#include "nvm_access.h"
#include "core_mesh_handler.h"
#include "app_mesh_handler.h"

/*============================================================================*
 *  Private Definitions
 *============================================================================*/
#ifndef DEBUG_ENABLE

/* Time for which the button bounce is expected */
#define BUTTON_DEBOUNCE_TIME           (20 * MILLISECOND)

/* Timer to check if button was pressed for 1 second */
#define BUTTON_ONE_SEC_PRESS_TIME      (1 * SECOND)

#define KEY_PRESSED  (TRUE)

#define KEY_RELEASED (FALSE)

/* Light Level Increment/Decrement Step Size */
#define LEVEL_STEP_SIZE               (4)

/* Maximum and Minimum light levels */
#define MAX_LEVEL                     (255)
#define MIN_LEVEL                     (0)

/*============================================================================*
 *  Private Data
 *============================================================================*/
/* Switch Button States. */
static bool     onButtonState  = KEY_RELEASED;
static bool     incButtonState = KEY_RELEASED;
static bool     decButtonState = KEY_RELEASED;
static timer_id oneSecTimerId  = TIMER_INVALID;

static uint8    switch_cmd_tid = 1;

/* Toggle switch de-bounce timer id */
timer_id        debounce_tid;

static uint8    brightness_level;

#ifndef CSR101x_A05
static pio_mask_t sw3_mask, sw2_mask, sw4_mask;
#endif

/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/
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
    CSRMESH_POWER_SET_STATE_T power_state;
    CSRMESH_LIGHT_SET_LEVEL_T light_level;

    if( tid == debounce_tid)
    {
        debounce_tid = TIMER_INVALID;

#ifdef CSR101x_A05
        /* Enable PIO Events again */
        PioSetEventMask(BUTTONS_BIT_MASK, pio_event_mode_both);
#else
        PioSetEventMultiple(sw3_mask,
                            pio_event_mode_both | pio_event_mode_wake_both);
        PioSetEventMultiple(sw2_mask, 
                            pio_event_mode_both | pio_event_mode_wake_both);
        PioSetEventMultiple(sw4_mask, 
                            pio_event_mode_both | pio_event_mode_wake_both);
#endif

        /* If PIO State is same as before starting de-bounce timer,
         * we have a valid event.
         */
        if ((PioGet(SW3_PIO) == FALSE) && (incButtonState == KEY_RELEASED))
        {
            /* Set State and increment level */
            incButtonState = KEY_PRESSED;
            if (brightness_level < (MAX_LEVEL - LEVEL_STEP_SIZE))
            {
                brightness_level += LEVEL_STEP_SIZE;
            }
            else
            {
                brightness_level = MAX_LEVEL;
            }

            /* Start 1 second timer */
            startOneSecTimer = TRUE;
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
            if (brightness_level > LEVEL_STEP_SIZE)
            {
                brightness_level -= LEVEL_STEP_SIZE;
            }
            else
            {
                brightness_level = MIN_LEVEL;
            }

            /* Start 1 second timer */
            startOneSecTimer = TRUE;
        }
        else if ((PioGet(SW2_PIO) == TRUE) && (decButtonState == KEY_PRESSED))
        {
            /* Set state to KEY RELEASE */
            decButtonState = KEY_RELEASED;
            update_nvm = TRUE;
        }

        if ((PioGet(SW4_PIO) == FALSE) && (onButtonState == KEY_RELEASED))
        {
            /* Set Button State */
            onButtonState = KEY_PRESSED;
            power_state.state = csr_mesh_power_state_on;
            power_state.tid = switch_cmd_tid++;
            AppPowerSetState(&power_state, FALSE);
        }
        else if ((PioGet(SW4_PIO) == TRUE) && (onButtonState == KEY_PRESSED))
        {
            /* Set state to KEY RELEASE */
            onButtonState = KEY_RELEASED;
            power_state.state = csr_mesh_power_state_off;
            power_state.tid = switch_cmd_tid++;
            AppPowerSetState(&power_state, FALSE);
        }

        /* Send Light Command and Create One Second Timer when flag is set */
        if (startOneSecTimer)
        {
            light_level.level = brightness_level;
            light_level.tid = switch_cmd_tid++;
            AppLightSetLevel(&light_level,FALSE);

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
            if (brightness_level < (MAX_LEVEL - (5*LEVEL_STEP_SIZE)))
            {
                brightness_level += (5*LEVEL_STEP_SIZE);
            }
            else
            {
                brightness_level = MAX_LEVEL;
            }

            light_level.level = brightness_level;
            light_level.tid = switch_cmd_tid++;
            AppLightSetLevel(&light_level, FALSE);

            oneSecTimerId = TimerCreate(BUTTON_ONE_SEC_PRESS_TIME, TRUE,
                                                    handleButtonDebounce);
        }

        /* Key has been held Pressed for a second now */
        if ((PioGet(SW2_PIO) == FALSE) && (decButtonState == KEY_PRESSED))
        {
            if (brightness_level > (5*LEVEL_STEP_SIZE))
            {
                brightness_level -= (5*LEVEL_STEP_SIZE);
            }
            else
            {
                brightness_level = MIN_LEVEL;
            }

            light_level.level = brightness_level;
            light_level.tid = switch_cmd_tid++;
            AppLightSetLevel(&light_level, FALSE);

            oneSecTimerId = TimerCreate(BUTTON_ONE_SEC_PRESS_TIME, TRUE,
                                                    handleButtonDebounce);
        }
    }

    /* Restart the tid */
    switch_cmd_tid  = switch_cmd_tid > 255? 0: switch_cmd_tid;

    /* Update NVM if required */
    if (update_nvm)
    {
        WriteSwitchDataOntoNVM();
    }
}


/*============================================================================*
 *  Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      SwitchHardwareInit
 *
 *  DESCRIPTION
 *      This function initializes the switch hardware, like PIO, interface etc.
 *
 * PARAMETERS
 *      Nothing.
 *
 * RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
extern void SwitchHardwareInit(void)
{
    IOTSwitchInit();

    /* Read the satus of the SW4 PIO and set the position accordingly */
    if(PioGet(SW4_PIO) == FALSE)
    {
        onButtonState = KEY_PRESSED;
    }
}
#ifdef CSR101x_A05
/*----------------------------------------------------------------------------*
 *  NAME
 *      HandlePIOChangedEvent
 *
 *  DESCRIPTION
 *      This function handles PIO Changed event
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void HandlePIOChangedEvent(pio_changed_data *data)
{
    bool start_timer = FALSE;
    uint32 pio_changed = data->pio_cause;

    /* When switch is not associated in stand alone mode,
     * don't send any CSR Mesh events.
     */
    if (AppGetAssociatedState() != app_state_associated)
    {
        return;
    }

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

        if (((PioGet(SW4_PIO) == FALSE) && (onButtonState== KEY_RELEASED)) ||
            ((PioGet(SW4_PIO) == TRUE) && (onButtonState == KEY_PRESSED)))
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
#else
/*----------------------------------------------------------------------------*
 *  NAME
 *      HandlePIOChangedEvent
 *
 *  DESCRIPTION
 *      This function handles PIO Changed event
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void HandlePIOChangedEvent(pio_changed_data *data)
{
    bool start_timer = FALSE;
    pio_changed_data *event_data;
    uint16 sw3_index, sw2_index, sw4_index;

    /* When switch is not associated in stand alone mode,
     * don't send any CSR Mesh events.
     */
    if (AppGetAssociatedState() != app_state_associated)
    {
        return;
    }

    /* The PIO changed data is defined by struct pio_changed_data */
    event_data = (pio_changed_data *)data;

    /* Get the IOT switch mask */
    GetIOTSwitchMask(SW3_PIO, &sw3_mask, &sw3_index);
    GetIOTSwitchMask(SW2_PIO, &sw2_mask, &sw2_index);
    GetIOTSwitchMask(SW4_PIO, &sw4_mask, &sw4_index);

    /* If the PIO event comes from the button */
    if((event_data->pio_cause.mask[sw3_index] & sw3_mask.mask[sw3_index]) ||
       (event_data->pio_cause.mask[sw2_index] & sw2_mask.mask[sw2_index]) ||
       (event_data->pio_cause.mask[sw4_index] & sw4_mask.mask[sw4_index]))
    {
        start_timer = TRUE;
    }

    if (start_timer)
    {
        /* Disable further Events */
        PioSetEventMultiple(sw3_mask, pio_event_mode_disable);
        PioSetEventMultiple(sw2_mask, pio_event_mode_disable);
        PioSetEventMultiple(sw4_mask, pio_event_mode_disable);

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
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      ReadSwitchDataFromNVM
 *
 *  DESCRIPTION
 *      This function reads the switch data from NVM.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void ReadSwitchDataFromNVM(void)
{
    Nvm_Read((uint16 *)&brightness_level, 
              sizeof(uint16),
              NVM_OFFSET_SWITCH_STATE);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      WriteSwitchDataOntoNVM
 *
 *  DESCRIPTION
 *      This function writes switch data onto NVM.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void WriteSwitchDataOntoNVM(void)
{
    Nvm_Write((uint16 *)&brightness_level, 
               sizeof(uint16),
               NVM_OFFSET_SWITCH_STATE);
}

#endif /* DEBUG_ENABLE */

