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

/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "user_config.h"
#include "nvm_access.h"
#include "main_app.h"
#include "app_mesh_handler.h"
#include "csr_mesh_model_common.h"
#include "app_util.h"
/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Service ID for CSRmesh Adverts is 0xFEF1. */
#define MTL_ID_CODE                    (0xFEF1)

/* CSRmesh Advert Data */
uint8 mesh_ad_data[3] =                {(AD_TYPE_SERVICE_DATA_UUID_16BIT),
                                       (MTL_ID_CODE & 0x00FF),
                                       ((MTL_ID_CODE & 0xFF00 ) >> 8)};

#define TX_QUEUE_SIZE                  (8)
#define DEFAULT_SCAN_DUTY_CYCLE        (1000)
#define DEFAULT_ADV_INTERVAL           (90 * MILLISECOND)
#define DEFAULT_ADV_TIME               (600 * MILLISECOND)
#define ONE_SHOT_ADV_TIME              (5 * MILLISECOND)
#define DEFAULT_ADDR_TYPE              (ls_addr_type_random)
#define DEVICE_REPEAT_COUNT            (DEFAULT_ADV_TIME)/(DEFAULT_ADV_INTERVAL\
                                        + ONE_SHOT_ADV_TIME)
#define RELAY_REPEAT_COUNT             (DEVICE_REPEAT_COUNT / 2)
#define DEFAULT_MIN_SCAN_SLOT          (0x0004)

/*============================================================================*
 *  Private Data
 *============================================================================*/

/* Declare buffer for Mesh Tx queue */
CSR_SCHED_MESH_TX_BUFFER_T             tx_queue_buffer[8];

static CSR_SCHED_LE_PARAMS_T           le_params;

/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/

/*-----------------------------------------------------------------------------*
 *  NAME
 *      setMeshConfigParams
 *
 *  DESCRIPTION
 *      This function sets the configuration parameters for csrmesh.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void setMeshConfigParams(CSR_SCHED_MESH_LE_PARAM_T *mesh_le_params)
{
    mesh_le_params->is_le_bearer_ready           = TRUE;
    mesh_le_params->tx_param.device_repeat_count = DEVICE_REPEAT_COUNT;
    mesh_le_params->tx_param.relay_repeat_count  = RELAY_REPEAT_COUNT;
    mesh_le_params->tx_param.tx_queue_size       = TX_QUEUE_SIZE;
    mesh_le_params->tx_param.tx_queue_ptr        = tx_queue_buffer;
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      setGenericConfigParams
 *
 *  DESCRIPTION
 *      This function sets the generic configuration parameters for csrmesh.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void setGenericConfigParams(
                            CSR_SCHED_GENERIC_LE_PARAM_T *generic_le_params)
{
    generic_le_params->scan_param_type      = CSR_SCHED_SCAN_DUTY_PARAM;
    generic_le_params->scan_param.scan_duty_param.scan_duty_cycle
                                            = DEFAULT_SCAN_DUTY_CYCLE;
    generic_le_params->scan_param.scan_duty_param.min_scan_slot   
                                            = DEFAULT_MIN_SCAN_SLOT;
    generic_le_params->advertising_interval = DEFAULT_ADV_INTERVAL;
    generic_le_params->advertising_time     = DEFAULT_ADV_TIME;
    generic_le_params->addr_type            = DEFAULT_ADDR_TYPE;
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      setLeConfigParams
 *
 *  DESCRIPTION
 *      This function sets the LE configuration parameters for csrmesh.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void setLeConfigParams(CSR_SCHED_LE_PARAMS_T *le_param)
{
    setMeshConfigParams(&le_param->mesh_le_param);
    setGenericConfigParams(&le_param->generic_le_param);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      readPersistentStore
 *
 *  DESCRIPTION
 *      This function is used to initialize and read NVM data
 *
 *  RETURNS/MODIFIES
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void readPersistentStore(void)
{
    /* NVM offset for supported services */
    uint16 nvm_sanity = 0xffff;
    uint16 app_nvm_version = 0;

    /* Read the sanity word */
    Nvm_Read(&nvm_sanity, sizeof(nvm_sanity),
             NVM_OFFSET_SANITY_WORD);

    /* Read the Application NVM version */
    Nvm_Read(&app_nvm_version, 1, NVM_OFFSET_APP_NVM_VERSION);

    if(g_app_nvm_fresh == FALSE && app_nvm_version == APP_NVM_VERSION )
    {
        /* Nothing to do */
    }
    else
    {
        /* Either the NVM Sanity is not valid or the App Version has changed */
        if( g_app_nvm_fresh == TRUE)
        {
            /* NVM Sanity check failed means either the device is being brought
             * up for the first time or memory has got corrupted in which case
             * discard the data and start fresh.
             */
            nvm_sanity = NVM_SANITY_MAGIC;

            /* Store new version of the NVM */
            app_nvm_version = APP_NVM_VERSION;
            Nvm_Write(&app_nvm_version, 1, NVM_OFFSET_APP_NVM_VERSION);

            /* Write NVM Sanity word to NVM. Make sure to write the sanity word
             * at the end after all other NVM data is written.
             * This helps in avoiding unexpected application behaviour in case 
             * of a device reset after sanity word is written but other NVM info
             * is not written
             */
            Nvm_Write(&nvm_sanity, sizeof(nvm_sanity), NVM_OFFSET_SANITY_WORD);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CSRmeshAppProcessMeshEvent
 *
 *  DESCRIPTION
 *   The CSRmesh™ stack calls this call-back to notify asynchronous 
 *   events to applications.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void CSRmeshAppProcessMeshEvent(CSR_MESH_APP_EVENT_DATA_T 
                                                            eventDataCallback)
{

}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      appUpdateBearerState
 *
 *  DESCRIPTION
 *      This function updates the relay and promiscuous mode of the GATT and
 *      and the LE Advert bearers
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void appUpdateBearerState(CSR_MESH_TRANSMIT_STATE_T *p_bearer_state)
{
    CSR_MESH_APP_EVENT_DATA_T ret_evt_data;
    CSR_MESH_TRANSMIT_STATE_T ret_bearer_state;

    ret_evt_data.appCallbackDataPtr = &ret_bearer_state;
    CSRmeshSetTransmitState(p_bearer_state, &ret_evt_data);
}

/*============================================================================*
 *  Public Function Implemtations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppMeshInit
 *
 *  DESCRIPTION
 *      This function confirms that the user store has been initialised
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void AppMeshInit(void)
{
    CSRmeshResult result = CSR_MESH_RESULT_FAILURE;
    CSR_MESH_TRANSMIT_STATE_T   bearer_tx_state;

    /* Set LE Config Params */
    setLeConfigParams(&le_params);
    CSRSchedSetConfigParams(&le_params);

    /* Start ADV GATT Scheduler */
    CSRSchedStart();

    readPersistentStore();

    result = CSRmeshInit(CSR_MESH_NON_CONFIG_DEVICE);

    CSRmeshRegisterAppCallback(CSRmeshAppProcessMeshEvent);

    if(result == CSR_MESH_RESULT_SUCCESS)
    {
        /* Start CSRmesh */
        result = CSRmeshStart();

        if(result == CSR_MESH_RESULT_SUCCESS)
        {
            bearer_tx_state.bearerPromiscuous = 
                               LE_BEARER_ACTIVE | GATT_SERVER_BEARER_ACTIVE;
            bearer_tx_state.bearerEnabled =  
                                LE_BEARER_ACTIVE | GATT_SERVER_BEARER_ACTIVE;
            bearer_tx_state.bearerRelayActive = 
                                LE_BEARER_ACTIVE | GATT_SERVER_BEARER_ACTIVE;

            bearer_tx_state.maspRelayEnable = TRUE;
            bearer_tx_state.relay.enable = TRUE;
            bearer_tx_state.relay.netId = CSR_MESH_DEFAULT_NETID;

            /* Update relay and promiscuous settings as per device state */
            appUpdateBearerState(&bearer_tx_state);
        }
    }
}

#ifdef NVM_TYPE_FLASH
/*----------------------------------------------------------------------------*
 *  NAME
 *      WriteApplicationAndServiceDataToNVM
 *
 *  DESCRIPTION
 *      This function writes the application data to NVM. This function should
 *      be called on getting nvm_status_needs_erase
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void WriteApplicationAndServiceDataToNVM(void)
{
    uint16 nvm_sanity = NVM_SANITY_MAGIC;
    uint16 app_nvm_version = APP_NVM_VERSION;

    /* Write NVM sanity word to the NVM */
    Nvm_Write(&nvm_sanity, sizeof(nvm_sanity), NVM_OFFSET_SANITY_WORD);

    /* Store new version of the NVM */
    Nvm_Write(&app_nvm_version, 1, NVM_OFFSET_APP_NVM_VERSION);
}
#endif /* NVM_TYPE_FLASH */

