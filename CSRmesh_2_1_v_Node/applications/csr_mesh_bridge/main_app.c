/******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      main_app.c
 *
 *  DESCRIPTION
 *      This file implements the CSR Mesh application.
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/
#ifdef CSR101x_A05
#include <ls_app_if.h>
#include <config_store.h>
#else
#include <uart.h>           /* Functions to interface with the UART */
#include <uart_sdk.h>       /* Enums to interface with the UART */
#include <configstore_api.h>
#include <configstore_id.h>
#endif /* !CSR101x_A05 */
#include <timer.h>
#include <uart.h>
#include <pio.h>
#include <nvm.h>
#include <debug.h>
/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "app_debug.h"
#include "iot_hw.h"
#include "battery_hw.h"
#include "nvm_access.h"
#include "main_app.h"
#include "gap_service.h"
#include "gatt_service.h"
#include "mesh_control_service.h"
#include "advertisement_handler.h"
#include "connection_handler.h"
#include "mesh_control_service.h"
#include "conn_param_update.h"
#ifdef CSR101x_A05
#include "csr_ota_service.h"
#else
#include "gaia_service.h"
#ifdef GAIA_OTAU_SUPPORT
#include "gaia_otau_api.h"
#include "app_otau_handler.h"
#endif /* GAIA_OTAU_SUPPORT  */
#endif /* CSR101x_A05 */

/*============================================================================*
 *  Private Definitions
 *============================================================================*/
#ifndef CSR101x_A05
/* Standard setup of CSR102x boards */
#define UART_PIO_TX 8
#define UART_PIO_RX 9
#define UART_PIO_RTS PIO_NONE
#define UART_PIO_CTS PIO_NONE
#endif /* !CSR101x_A05 */

/* NVM Store ID */
#define NVM_ID                          (2)

/*============================================================================*
 *  Private Data
 *===========================================================================*/
/* Declare space for application timers. */
static uint16 app_timers[SIZEOF_APP_TIMER * MAX_APP_TIMERS];

/*============================================================================*
 *  Public Data
 *============================================================================*/

/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/
/* UART Receive callback */
#if defined(CSR101x_A05) && defined(DEBUG_ENABLE)
static uint16 UartDataRxCallback ( void* p_data, uint16 data_count,
                                   uint16* p_num_additional_words );
#endif /* defined(CSR101x_A05) && defined(DEBUG_ENABLE) */

/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/
/*-----------------------------------------------------------------------------*
 *  NAME
 *      UartDataRxCallback
 *
 *  DESCRIPTION
 *      This callback is issued when data is received over UART. Application
 *      may ignore the data, if not required. For more information refer to
 *      the API documentation for the type "uart_data_out_fn"
 *
 *  RETURNS
 *      The number of words processed, return data_count if all of the received
 *      data had been processed (or if application don't care about the data)
 *
 *----------------------------------------------------------------------------*/
#if defined(CSR101x_A05) && defined(DEBUG_ENABLE)
static uint16 UartDataRxCallback ( void* p_data, uint16 data_count,
        uint16* p_num_additional_words )
{
    /* Application needs 1 additional data to be received */
    *p_num_additional_words = 1;

    return data_count;
}
#endif /* defined(CSR101x_A05) && defined(DEBUG_ENABLE) */

#ifndef CSR101x_A05
/*----------------------------------------------------------------------------*
 *  NAME
 *      configStoreProcessEvent
 *
 *  DESCRIPTION
 *      This function handles the config store messages
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void configStoreProcessEvent(msg_t *msg)
{
    configstore_msg_t *cs_msg = (configstore_msg_t*)msg;
    switch(msg->header.id)
    {
        case CONFIG_STORE_READ_KEY_CFM:
        {
            configstore_read_key_cfm_t *read_key_cfm =
                    (configstore_read_key_cfm_t*)&cs_msg->body;

            if (STATUS_SUCCESS == read_key_cfm->status)
            {
                if(CS_ID_APP_STORE_ID == read_key_cfm->id)
                {
#ifdef GAIA_OTAU_SUPPORT
                    GaiaOtauConfigStoreMsg(msg);
#endif /* GAIA_OTAU_SUPPORT */
                }
            }
        }
        break;
        default:
        break;
    }
}
#endif

/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      InitialiseAppSuppServices
 *
 *  DESCRIPTION
 *      This function initialises the application supported services.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void InitAppSupportedServices(void)
{
#ifndef CSR101x_A05
    g_app_nvm_offset = NVM_OFFSET_GAIA_OTA_SERVICE;
    /* Initialize the Gaia OTA service. */
#ifdef RESET_NVM
    GAIAInitServerService(g_gaia_nvm_fresh, &g_app_nvm_offset);
#else
    GAIAInitServerService(g_app_nvm_fresh, &g_app_nvm_offset);
#endif /* RESET_NVM */
#ifdef GAIA_OTAU_SUPPORT
    AppGaiaOtauInit();
#endif /* GAIA_OTAU_SUPPORT */
    g_app_nvm_offset = NVM_OFFSET_MESH_APP_SERVICES;
#else
#ifdef OTAU_BOOTLOADER
    /* Initialize the OTA service. */
    OtaInitServerService(g_app_nvm_fresh, &g_app_nvm_offset);
#endif
#endif
    
    /* Initialize the GATT service. */
    GattExtInitServerService(g_app_nvm_fresh, &g_app_nvm_offset, TRUE);

    /* Initialize the GAP service. */
    GapInitServerServiceNoBond();

    /* Initialize the Mesh control service */
    MeshControlInitServerService(g_app_nvm_fresh, &g_app_nvm_offset);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppDataInit
 *
 *  DESCRIPTION
 *      This function is called to initialise CSRmesh application
 *      data structure.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void AppDataInit(void)
{
    /* Stop the connection parameter update if its in progress */
    StopConnParamsUpdate();

    /* Initialises the GATT Data */
    InitialiseGattData();

    /* Initialize the Mesh Control Service Data Structure */
    MeshControlServiceDataInit();
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      AppInit
 *
 *  DESCRIPTION
 *      This user application function is called after a power-on reset
 *      (including after a firmware panic), after a wakeup from Hibernate or
 *      Dormant sleep states, or after an HCI Reset has been requested.
 *
 *      The last sleep state is provided to the application in the parameter.
 *
 *      NOTE: In the case of a power-on reset, this function is called
 *      after app_power_on_reset().
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
void AppInit(sleep_state last_sleep_state)
{
    /* Initialise the application timers */
    TimerInit(MAX_APP_TIMERS, (void*)app_timers);

#ifdef DEBUG_ENABLE
#ifdef CSR101x_A05
    /* Initialize UART and configure with
     * default baud rate and port configuration.
     */
    DebugInit(UART_BUF_SIZE_BYTES_256, UartDataRxCallback, NULL);

    /* UART Rx threshold is set to 1,
     * so that every byte received will trigger the rx callback.
     */
    UartRead(1, 0);
#else
    /* Configuration structure for the UART */
    uart_pio_pins_t uart;

    /* Standard setup of CSR102x boards */
    uart.rx  = UART_PIO_RX;
    uart.tx  = UART_PIO_TX;
    uart.rts = UART_PIO_RTS;
    uart.cts = UART_PIO_CTS;
    
    /* Initialise Default UART communications */
    DebugInit(1, UART_RATE_921K6, 0, &uart);
#endif /* CSR101x_A05 */
#endif /* DEBUG_ENABLE */

    /* Initialise Light and turn it off. */
    IOTLightControlDeviceInit();
    IOTLightControlDevicePower(FALSE);

    /* Initialize the nvm offset to max words taken by application. The CM is
     * stores its data after this.
     */
    g_app_nvm_offset = NVM_OFFSET_SANITY_WORD;

    /* Initialise the NVM. AppNvmReady is called when the NVM initialisation
     * complete.
     */
#ifdef CSR101x_A05
    Nvm_Init(NVM_SANITY_MAGIC, &g_app_nvm_offset);
#else
    Nvm_Init(NVM_ID,NVM_SANITY_MAGIC, &g_app_nvm_offset);
#endif    
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppNvmReady
 *
 *  DESCRIPTION
 *      This function is called when the NVM is initialised
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void AppNvmReady(bool nvm_fresh, uint16 nvm_offset)
{
    /* Save the NVM data */
    g_app_nvm_fresh = nvm_fresh;
#ifdef CSR101x_A05
    g_app_nvm_offset = NVM_MAX_APP_MEMORY_WORDS;
#else
    g_app_nvm_offset = NVM_OFFSET_CM_INITIALISATION;
/* If RESET_NVM flag is set, this means APP/Stack NVM offsets have changed 
 * between the  releases.In order for GAIA OTAu to complete successfully if it
 * is in post reboot phase of OTA, force the GAIA service to read from the old 
 * offset.All NVM data will be lost after this update.
 * Note: GAIA service offsets should be still same between the releases.
 */
#ifdef RESET_NVM
    if(GAIAInPostRebootPhase())
    {
        g_app_nvm_fresh = TRUE;
        g_gaia_nvm_fresh = FALSE;
    }
    else
    {
        g_gaia_nvm_fresh = g_app_nvm_fresh;
    }
#endif /* RESET_NVM */  
#endif

    /* Initilize CM */
    AppCMInit(g_app_nvm_fresh, &g_app_nvm_offset);
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      AppProcesSystemEvent
 *
 *  DESCRIPTION
 *      This user application function is called whenever a system event, such
 *      as a battery low notification, is received by the system.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
extern void AppProcessSystemEvent(sys_event_id id, void *data)
{
    switch (id)
    {
        case sys_event_pio_changed:
        {
        }
        break;

        case sys_event_battery_low:
        {
        }
        break;

        default:
        break;
    }
}

#ifndef CSR101x_A05
/*----------------------------------------------------------------------------*
 *  NAME
 *      AppProcessEvent
 *
 *  DESCRIPTION
 *      Handles the system events
 *
 *  RETURNS
 *     status_t: STATUS_SUCCESS if successful
 *
 *----------------------------------------------------------------------------*/

status_t AppProcessEvent(msg_t *msg)
{
    switch(GET_GROUP_ID(msg->header.id))
    {
        case SM_GROUP_ID:
        case LS_GROUP_ID:
        case GATT_GROUP_ID:
        {
            /* CM Handles the SM, LS and GATT Messages */
            CMProcessMsg(msg);
        }
        break;

        case USER_STORE_GROUP_ID:
        {
            NvmProcessEvent(msg);
        }
        break;
        
        case CONFIG_STORE_GROUP_ID:
        {
            configStoreProcessEvent(msg);
        }
        break;
        
#ifdef GAIA_OTAU_SUPPORT
        case STORE_UPDATE_GROUP_ID:
        GaiaOtauHandleStoreUpdateMsg(GetConnectedDeviceId(), (store_update_msg_t*)msg );
        break;
#endif

        default:
        break;
    }    
    return STATUS_SUCCESS;
}

#else
/*----------------------------------------------------------------------------*
 *  NAME
 *      AppProcessLmEvent
 *
 *  DESCRIPTION
 *      This user application function is called whenever a LM-specific event is
 *      received by the system.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern bool AppProcessLmEvent(lm_event_code event_code,
                              LM_EVENT_T *p_event_data)
{
    /* CM Handles the SM, LS and GATT Messages */
    CMProcessMsg(event_code, p_event_data);
    return TRUE;
}

/*----------------------------------------------------------------------------*
 * NAME
 *   AppPowerOnReset
 *
 * DESCRIPTION
 *   This user application function is called just after a power-on reset
 *   (including after a firmware panic), or after a wakeup from Hibernate or
 *   Dormant sleep states.
 *
 *   At the time this function is called, the last sleep state is not yet
 *   known.
 *
 *   NOTE: this function should only contain code to be executed after a
 *   power-on reset or panic. Code that should also be executed after an
 *   HCI_RESET should instead be placed in the AppInit() function.
 *
 * RETURNS
 *   Nothing
 *
 *----------------------------------------------------------------------------*/
void AppPowerOnReset(void)
{
     /* Code that is only executed after a power-on reset or firmware panic 
      * should be implemented here.
      */
}
#endif
