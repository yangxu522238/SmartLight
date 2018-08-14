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
#include <mem.h>
#include <random.h>
/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "user_config.h"
#include "nvm_access.h"
#include "app_debug.h"
#include "app_gatt.h"
#include "main_app.h"
#include "iot_hw.h"
#include "appearance.h"
#include "iot_hw.h"
#include "beacon_model_handler.h"
#include "beacon_proxy_model_handler.h"
#include "battery_model_handler.h"
#include "attention_model_handler.h"
#include "time_model_handler.h"
#include "action_model_handler.h"
#include "data_model_handler.h"
#include "app_mesh_handler.h"
#include "ping_server.h"
#include "largeobjecttransfer_model_handler.h"

/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* The User key index where the application config flags are stored */
#define CSKEY_INDEX_USER_FLAGS         (0)

/* Bluetooth SIG Organization identifier for CSRmesh device appearance */
#define APPEARANCE_ORG_BLUETOOTH_SIG   (0)

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
CSR_SCHED_MESH_TX_BUFFER_T tx_queue_buffer[8];

/* Application VID,PID and Version. */
CSR_MESH_VID_PID_VERSION_T vid_pid_info =
{
    .vendor_id  = APP_VENDOR_ID,
    .product_id = APP_PRODUCT_ID,
    .version    = APP_VERSION,
};

/* Device Apprearance. */
CSR_MESH_DEVICE_APPEARANCE_T appearance = {"Proxy2.1",
                                           {APPEARANCE_ORG_BLUETOOTH_SIG,
                                           APPEARANCE_CSRMESH_BEACON_PROXY_VALUE,
                                           0x00000000}};

CSR_SCHED_LE_PARAMS_T                           le_params;

MESH_HANDLER_DATA_T                             g_mesh_handler_data;

#ifdef ENABLE_BEACON_MODEL
static BEACON_HANDLER_DATA_T                    g_beacon_handler_data;
#endif /* ENABLE_BEACON_MODEL */

#ifdef ENABLE_BEACON_PROXY_MODEL
static BEACON_PROXY_HANDLER_DATA_T              g_beacon_proxy_data;
#endif /* ENABLE_BEACON_PROXY_MODEL */

#ifdef ENABLE_ATTENTION_MODEL
static ATTENTION_HANDLER_DATA_T                 g_attention_handler_data;
#endif /* ENABLE_ATTENTION_MODEL */

#ifdef ENABLE_BATTERY_MODEL
static BATTERY_HANDLER_DATA_T                   g_battery_handler_data;
#endif /* ENABLE_BATTERY_MODEL */

#ifdef ENABLE_TIME_MODEL
static TIME_HANDLER_DATA_T                      g_time_handler_data;
#endif /* ENABLE_TIME_MODEL */

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

/*-----------------------------------------------------------------------------*
 *  NAME
 *      initializeSupportedModelData
 *
 *  DESCRIPTION
 *      This function initializes the mesh model data used by the application.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void initializeSupportedModelData(void)
{
#ifdef ENABLE_BEACON_MODEL
        /* Initialize the beacon  model */
        BeaconModelDataInit(&g_beacon_handler_data);
#endif /* ENABLE_BEACON_MODEL */

#ifdef ENABLE_BEACON_PROXY_MODEL
        /* Initialize the beacon proxy model */
        BeaconProxyModelDataInit(&g_beacon_proxy_data);
#endif /* ENABLE_BEACON_PROXY_MODEL */

#ifdef ENABLE_ATTENTION_MODEL
        /* Initialize the attention model */
        AttentionModelDataInit(&g_attention_handler_data);
#endif /* ENABLE_ATTENTION_MODEL */

#ifdef ENABLE_BATTERY_MODEL
        /* Initialize the battery model */
        BatteryModelDataInit(&g_battery_handler_data);
#endif /* ENABLE_BATTERY_MODEL */

#ifdef ENABLE_TIME_MODEL
        /* Initialize Time Model */
        TimeModelDataInit(&g_time_handler_data);
#endif /* ENABLE_TIME_MODEL */

#ifdef ENABLE_ACTION_MODEL
        /* Initialize Action Model */
        ActionModelDataInit();
#endif /* ENABLE_ACTION_MODEL */
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      initializeSupportedModels
 *
 *  DESCRIPTION
 *      This function initializes all the mesh models used by the application.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void initializeSupportedModels(void)
{
#ifdef ENABLE_BEACON_MODEL
        /* Initialize the beacon model */
        BeaconModelHandlerInit(CSR_MESH_DEFAULT_NETID, 
                               beacon_model_groups,
                               MAX_MODEL_GROUPS);
#endif /* ENABLE_BEACON_MODEL */

#ifdef ENABLE_BEACON_PROXY_MODEL
        /* Initialize the beacon proxy model */
        BeaconProxyModelHandlerInit(CSR_MESH_DEFAULT_NETID, NULL, 0);
#endif /* ENABLE_BEACON_PROXY_MODEL */

#ifdef ENABLE_ATTENTION_MODEL
        /* Initialize the attention model */
        AttentionModelHandlerInit(CSR_MESH_DEFAULT_NETID, 
                                  attention_model_groups, 
                                  MAX_MODEL_GROUPS);
#endif /* ENABLE_ATTENTION_MODEL */

#ifdef ENABLE_BATTERY_MODEL
        /* Initialize the battery model */
        BatteryModelHandlerInit(CSR_MESH_DEFAULT_NETID, NULL, 0);
#endif /* ENABLE_BATTERY_MODEL */

#ifdef ENABLE_DATA_MODEL
        /* Initialize the data model */
        DataModelHandlerInit(CSR_MESH_DEFAULT_NETID, data_model_groups,
                             MAX_MODEL_GROUPS);
#endif /* ENABLE_DATA_MODEL */

#ifdef ENABLE_TIME_MODEL
        /* Initialize Time Model */
        TimeModelHandlerInit(CSR_MESH_DEFAULT_NETID, NULL, 0);
#endif /* ENABLE_TIME_MODEL */

#ifdef ENABLE_ACTION_MODEL
        /* Initialize Action Model */
        ActionModelHandlerInit(CSR_MESH_DEFAULT_NETID, NULL, 0);
#endif /* ENABLE_ACTION_MODEL */

#ifdef ENABLE_PING_MODEL
       /* Initialize Ping Model */
        PingModelInit(CSR_MESH_DEFAULT_NETID, NULL, 0, NULL);
#endif /* ENABLE_PING_MODEL */
        
#ifdef ENABLE_LOT_MODEL
       /* Initialize LOT Model */
        LotModelHandlerInit(CSR_MESH_DEFAULT_NETID,lot_model_groups, 
                              MAX_MODEL_GROUPS);
#endif /* ENABLE_LOT_MODEL */
}


/*-----------------------------------------------------------------------------*
 *  NAME
 *      initializeSupportedModelGroups
 *
 *  DESCRIPTION
 *      This function initialises the supported model groups.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void initializeSupportedModelGroups(void)
{
#ifdef ENABLE_ATTENTION_MODEL
    MemSet(attention_model_groups, 0x0000, sizeof(uint16)*MAX_MODEL_GROUPS);
#endif

#ifdef ENABLE_DATA_MODEL
    MemSet(data_model_groups, 0x0000, sizeof(uint16)*MAX_MODEL_GROUPS);
#endif
#ifdef ENABLE_BEACON_MODEL
    MemSet(beacon_model_groups, 0x0000, sizeof(uint16)*MAX_MODEL_GROUPS);
#endif

#ifdef ENABLE_LOT_MODEL
    MemSet(lot_model_groups, 0x0000, sizeof(uint16)*MAX_MODEL_GROUPS);
#endif
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      writeSupportedModelGroupsOntoNVM
 *
 *  DESCRIPTION
 *      This function writes the supported model groups onto NVM.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void writeSupportedModelGroupsOntoNVM(void)
{
#ifdef ENABLE_ATTENTION_MODEL
    Nvm_Write((uint16 *)attention_model_groups, sizeof(uint16)*MAX_MODEL_GROUPS,
                                                NVM_OFFSET_ATT_MODEL_GROUPS);
#endif /* ENABLE_ATTENTION_MODEL */

#ifdef ENABLE_DATA_MODEL
    Nvm_Write((uint16 *)data_model_groups, sizeof(uint16)*MAX_MODEL_GROUPS,
                                                NVM_OFFSET_DATA_MODEL_GROUPS);
#endif /* ENABLE_DATA_MODEL */

#ifdef ENABLE_BEACON_MODEL
    Nvm_Write((uint16 *)beacon_model_groups, sizeof(uint16)*MAX_MODEL_GROUPS,
                                                NVM_OFFSET_BEACON_MODEL_GROUPS);
#endif /* ENABLE_BEACON_MODEL */
    
#ifdef ENABLE_LOT_MODEL
    Nvm_Write((uint16 *)lot_model_groups, sizeof(uint16)*MAX_MODEL_GROUPS,
                                                NVM_OFFSET_LOT_MODEL_GROUPS);
#endif /* ENABLE_LOT_MODEL */
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      readSupportedModelGroupsFromNVM
 *
 *  DESCRIPTION
 *      This function reads the supported model groups from NVM onto the global
 *      structure.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void readSupportedModelGroupsFromNVM(void)
{
#ifdef ENABLE_ATTENTION_MODEL
    /* Read assigned Groups IDs for Attention model from NVM */
    Nvm_Read((uint16 *)attention_model_groups, sizeof(uint16)*MAX_MODEL_GROUPS,
                                             NVM_OFFSET_ATT_MODEL_GROUPS);
#endif /* ENABLE_ATTENTION_MODEL */

#ifdef ENABLE_DATA_MODEL
    /* Read assigned Groups IDs for Data model from NVM */
    Nvm_Read((uint16 *)data_model_groups, sizeof(uint16)*MAX_MODEL_GROUPS,
                                             NVM_OFFSET_DATA_MODEL_GROUPS);
#endif /* ENABLE_DATA_MODEL */

#ifdef ENABLE_BEACON_MODEL
    /* Read assigned Groups IDs for Beacon model from NVM */
    Nvm_Read((uint16 *)beacon_model_groups, sizeof(uint16)*MAX_MODEL_GROUPS,
                                             NVM_OFFSET_BEACON_MODEL_GROUPS);
#endif /* ENABLE_BEACON_MODEL */
    
#ifdef ENABLE_LOT_MODEL
    Nvm_Read((uint16 *)lot_model_groups, sizeof(uint16)*MAX_MODEL_GROUPS,
                                                NVM_OFFSET_LOT_MODEL_GROUPS);
#endif

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
    uint16 index;

#ifdef CSR101x_A05
    g_cskey_flags = CSReadUserKey(CSKEY_INDEX_USER_FLAGS);
#endif

    /* Read the sanity word */
    Nvm_Read(&nvm_sanity, sizeof(nvm_sanity), NVM_OFFSET_SANITY_WORD);

    /* Read the Application NVM version */
    Nvm_Read(&app_nvm_version, 1, NVM_OFFSET_APP_NVM_VERSION);

    if(g_app_nvm_fresh == FALSE && app_nvm_version == APP_NVM_VERSION)
    {
#ifdef ENABLE_BEACON_MODEL
        /* Read the Beacon Model Data */
        ReadBeaconModelDataFromNVM();
#endif

#ifdef ENABLE_BEACON_PROXY_MODEL
        /* Read the Beacon Model Data */
        ReadBeaconProxyModelDataFromNVM();
#endif

#ifdef ENABLE_TIME_MODEL
        /* Read the Time Model Data */
        ReadTimeModelDataFromNVM(NVM_OFFSET_TIME_INTERVAL);
#endif

#ifdef ENABLE_ACTION_MODEL
        /* Read the Action Model Data */
        ReadActionModelDataFromNVM(NVM_OFFSET_ACTION_MODEL_DATA);
#endif

        /* Read the saved bearer state from NVM */
        Nvm_Read((uint16 *)&g_mesh_handler_data.bearer_tx_state, 
                  sizeof(CSR_MESH_BEARER_STATE_DATA_T),
                  NVM_OFFSET_BEARER_STATE);

        /* Read Stored TTL value from NVM */
        Nvm_Read((uint16 *)&g_mesh_handler_data.ttl_value,
                sizeof(g_mesh_handler_data.ttl_value),
                NVM_OFFSET_TTL_VALUE);
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

            /* The device will not be associated as it is coming up for the
             * first time
             */
            g_mesh_handler_data.assoc_state = app_state_not_associated;

            /* Write association state to NVM */
            Nvm_Write((uint16 *)&g_mesh_handler_data.assoc_state,
                      sizeof(g_mesh_handler_data.assoc_state),
                      NVM_OFFSET_ASSOCIATION_STATE);

            if (g_cskey_flags & CSKEY_RANDOM_UUID_ENABLE_BIT)
            {
                uint16 uuid[UUID_LENGTH_WORDS];

                /* The flag is set so generate a random UUID and write to NVM */
                for (index = 0; index < UUID_LENGTH_WORDS; index++)
                {
                    uuid[index] = Random16();
                }
                Nvm_Write(uuid, UUID_LENGTH_WORDS,
                          CSR_MESH_NVM_DEVICE_UUID_OFFSET);

#ifdef USE_AUTHORISATION_CODE
                uint16 ac[AUTH_CODE_LENGTH_WORDS];

                /* The flag is set so generate a random UUID and write to NVM */
                for (index = 0; index < AUTH_CODE_LENGTH_WORDS; index++)
                {
                    ac[index] = Random16();
                }
                Nvm_Write(ac, AUTH_CODE_LENGTH_WORDS,
                          CSR_MESH_NVM_DEVICE_AUTH_CODE_OFFSET);
#endif /* USE_AUTHORISATION_CODE */
            }
            else
            {
#ifndef CSR101x
                /* Copy UUID and Authorization Code from CS Keys to Flash, if
                 * Random UUID is disabled in flags.
                 */
                Nvm_Write(cached_uuid, UUID_LENGTH_WORDS, CSR_MESH_NVM_DEVICE_UUID_OFFSET);

#ifdef USE_AUTHORISATION_CODE
                Nvm_Write(cached_auth_code, AUTH_CODE_LENGTH_WORDS, CSR_MESH_NVM_DEVICE_AUTH_CODE_OFFSET);
#endif /* USE_AUTHORISATION_CODE */

#endif /* !CSR101x */
            }

            /* Write NVM Sanity word to NVM. Make sure to write the sanity word
             * at the end after all other NVM data is written.
             * This helps in avoiding unexpected application behaviour in case 
             * of a device reset after sanity word is written but other NVM info
             * is not written
             */
            Nvm_Write(&nvm_sanity, sizeof(nvm_sanity), NVM_OFFSET_SANITY_WORD);
        }

        /* Initialize the new version of the NVM */
        app_nvm_version = APP_NVM_VERSION;

        /* All the persistent data below will be reset to default upon an
         * application update. If some of the data needs to be retained even
         * after an application update, it has to be moved within the sanity
         * word check
         */
#ifndef RESET_NVM
#if (SRC_SEQ_CACHE_SLOT_SIZE == 0)
        AppClearSeqCache();
#endif
#endif /* !RESET_NVM */

#ifdef ENABLE_BEACON_MODEL
        BeaconModelDataInit(NULL);
        WriteBeaconModelDataOntoNVM();
#endif

#ifdef ENABLE_BEACON_PROXY_MODEL
        BeaconProxyModelDataInit(NULL);
        WriteBeaconProxyModelDataOntoNVM();
#endif

#ifdef ENABLE_TIME_MODEL
        TimeModelDataInit(NULL);
        WriteTimeModelDataOntoNVM(NVM_OFFSET_TIME_INTERVAL);
#endif

#ifdef ENABLE_ACTION_MODEL
        ActionModelDataInit();
        WriteActionModelDataOntoNVM(NVM_OFFSET_ACTION_MODEL_DATA);
#endif

        /* Enable relay and bridge based on the CS User Key setting */
        g_mesh_handler_data.bearer_tx_state.bearerEnabled =  LE_BEARER_ACTIVE;
        if( g_cskey_flags & CSKEY_RELAY_ENABLE_BIT)
        {
            g_mesh_handler_data.bearer_tx_state.bearerRelayActive = 
                               LE_BEARER_ACTIVE | GATT_SERVER_BEARER_ACTIVE;
        }
        if( g_cskey_flags & CSKEY_BRIDGE_ENABLE_BIT)
        {
            g_mesh_handler_data.bearer_tx_state.bearerEnabled  |=
                                                  GATT_SERVER_BEARER_ACTIVE;
        }
        /* Enable promiscuous mode as the device is not associated. This
         * allows the device to relay MCP messages targeted for other
         * devices that are already associated
         */
        g_mesh_handler_data.bearer_tx_state.bearerPromiscuous = 
                                   LE_BEARER_ACTIVE | GATT_SERVER_BEARER_ACTIVE;
        /* Save the state to NVM */
        Nvm_Write((uint16 *)&g_mesh_handler_data.bearer_tx_state, 
                  sizeof(CSR_MESH_BEARER_STATE_DATA_T),
                  NVM_OFFSET_BEARER_STATE);

        g_mesh_handler_data.ttl_value = DEFAULT_TTL_VALUE;
        /* Save the ttl value onto NVM */
        Nvm_Write((uint16 *)&g_mesh_handler_data.ttl_value, 
                  sizeof(g_mesh_handler_data.ttl_value),
                  NVM_OFFSET_TTL_VALUE);

        /* Initialize model groups */
        initializeSupportedModelGroups();
        writeSupportedModelGroupsOntoNVM();


        /* Write app NVM version to NVM. Make sure to write the app nvm version
         * at the end after all other NVM data is written.
         * This helps in avoiding unexpected application behaviour in case 
         * of a device reset after sanity word is written but other NVM info
         * is not written
         */
        Nvm_Write(&app_nvm_version, 1, NVM_OFFSET_APP_NVM_VERSION);
    }

    /* Read all the supported model group information from the NVM */
    readSupportedModelGroupsFromNVM();

    /* Read association state from NVM */
    Nvm_Read((uint16 *)&g_mesh_handler_data.assoc_state,
              sizeof(g_mesh_handler_data.assoc_state),
              NVM_OFFSET_ASSOCIATION_STATE);
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

    /* Set LE Config Params */
    setLeConfigParams(&le_params);
    CSRSchedSetConfigParams(&le_params);

    /* Start ADV GATT Scheduler */
    CSRSchedStart();

    /* Don't wakeup on UART RX line 
    SleepWakeOnUartRX(FALSE); */

    /* Initialize the data structures in the models. This function should be 
     * called before the readPersistentStore() as the model handlers structures
     * could be poulated in the readPersistentStore() function.
     */
    initializeSupportedModelData();

    /* Read persistent storage */
    readPersistentStore();

    /* Register with CSR Mesh */
#ifdef USE_AUTHORISATION_CODE
    result = CSRmeshInit(CSR_MESH_NON_CONFIG_DEVICE_WITH_AUTH_CODE);
#else
    result = CSRmeshInit(CSR_MESH_NON_CONFIG_DEVICE);
#endif /* USE_AUTHORISATION_CODE */

    g_mesh_handler_data.appearance = &appearance;
    g_mesh_handler_data.vid_pid_info = &vid_pid_info;

    MeshHandlerInit(&g_mesh_handler_data);

    if(result == CSR_MESH_RESULT_SUCCESS)
    {
        /* Initialize all the models supported in the application. */
        initializeSupportedModels();

        /* Start CSRmesh */
        result = CSRmeshStart();

        if(result == CSR_MESH_RESULT_SUCCESS)
        {
            /* Post MASP_DEVICE_ID msg in Bearer TX queue */
            if(g_mesh_handler_data.assoc_state == app_state_not_associated)
            {
                if(g_app_nvm_fresh == TRUE)
                {
                    /* As the application's sanity has changed, the association 
                     * information in the core stack needs to be removed.
                     */
                    CSRmeshRemoveNetwork(CSR_MESH_DEFAULT_NETID);
                }
                /* Start sending device UUID adverts */
                InitiateAssociation();
            }
            else
            {
                DEBUG_STR("Beacon Proxy is associated\r\n");
                /* Light is already associated. Set the colour from NVM */
                RestoreLightState();

                /* Initialize the source sequence cache */
                AppInitializeSeqCache(FALSE);
            }
            /* Update relay and promiscuous settings as per device state */
            AppUpdateBearerState(&g_mesh_handler_data.bearer_tx_state);

            /* Update the TTL value onto mesh stack */
            CSRmeshSetDefaultTTL(g_mesh_handler_data.ttl_value);
        }
    }
    else
    {
        /* Registration has failed */
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HandleGroupSetMsg
 *
 *  DESCRIPTION
 *      This function handles the CSRmesh Group Assignment message. Stores
 *      the group_id at the given index for the model
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern bool HandleGroupSetMsg(CSR_MESH_GROUP_ID_RELATED_DATA_T msg)
{
#if defined (ENABLE_ATTENTION_MODEL) || defined (ENABLE_DATA_MODEL) || defined (ENABLE_BEACON_MODEL) || defined (ENABLE_LOT_MODEL)
    CSRMESH_MODEL_TYPE_T model = msg.model;
    uint8 index = msg.gpIdx;
    uint16 group_id = msg.gpId;
#endif
    bool update_lastetag = TRUE;

#ifdef ENABLE_ATTENTION_MODEL
    if(model == CSRMESH_ATTENTION_MODEL || model == CSRMESH_ALL_MODELS)
    {
        if(index < MAX_MODEL_GROUPS)
        {
            attention_model_groups[index] = group_id;

            /* Save to NVM */
            Nvm_Write(&attention_model_groups[index],
                      sizeof(uint16),
                      NVM_OFFSET_ATT_MODEL_GROUPS + index);
        }
        else
        {
            update_lastetag = FALSE;
        }
    }
#endif /* ENABLE_ATTENTION_MODEL */

#ifdef ENABLE_DATA_MODEL
    if(model == CSRMESH_DATA_MODEL || model == CSRMESH_ALL_MODELS)
    {
        if(index < MAX_MODEL_GROUPS)
        {
            data_model_groups[index] = group_id;

            /* Save to NVM */
            Nvm_Write(&data_model_groups[index],
                      sizeof(uint16),
                      NVM_OFFSET_DATA_MODEL_GROUPS + index);
        }
        else
        {
            update_lastetag = FALSE;
        }
    }
#endif /* ENABLE_DATA_MODEL */

#ifdef ENABLE_BEACON_MODEL
    if(model == CSRMESH_BEACON_MODEL || model == CSRMESH_ALL_MODELS)
    {
        if(index < MAX_MODEL_GROUPS)
        {
            beacon_model_groups[index] = group_id;

            /* Save to NVM */
            Nvm_Write(&beacon_model_groups[index],
                      sizeof(uint16),
                      NVM_OFFSET_BEACON_MODEL_GROUPS + index);
        }
        else
        {
            update_lastetag = FALSE;
        }
    }
#endif /* ENABLE_BEACON_MODEL */
    
#ifdef ENABLE_LOT_MODEL
    if(model == CSRMESH_LARGEOBJECTTRANSFER_MODEL || model == CSRMESH_ALL_MODELS)
    {
        if(index < MAX_MODEL_GROUPS)
        {
            lot_model_groups[index] = group_id;

            /* Save to NVM */
            Nvm_Write(&lot_model_groups[index],
                      sizeof(uint16),
                      NVM_OFFSET_LOT_MODEL_GROUPS + index);
        }
        else
        {
            update_lastetag = FALSE;
        }
    }
#endif /* ENABLE_LOT_MODEL */

    return update_lastetag;
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      RemoveAssociation
 *
 *  DESCRIPTION
 *      This function removes the associtation and moves back the device to 
 *      start associating again.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void RemoveAssociation(void)
{
    if (app_state_not_associated != g_mesh_handler_data.assoc_state)
    {
        /* clear the sequence cache present in mesh stack */
        AppClearSeqCache();

        /* API called to remove the network from the mesh stack */
        CSRmeshRemoveNetwork(CSR_MESH_DEFAULT_NETID);

        /* Enable relay and bridge based on the CS User Key setting */
        g_mesh_handler_data.bearer_tx_state.bearerEnabled =  LE_BEARER_ACTIVE;
        if( g_cskey_flags & CSKEY_RELAY_ENABLE_BIT)
        {
            g_mesh_handler_data.bearer_tx_state.bearerRelayActive = 
                               LE_BEARER_ACTIVE | GATT_SERVER_BEARER_ACTIVE;
        }
        if( g_cskey_flags & CSKEY_BRIDGE_ENABLE_BIT)
        {
            g_mesh_handler_data.bearer_tx_state.bearerEnabled  |=
                                                  GATT_SERVER_BEARER_ACTIVE;
        }

         /* Enable promiscuous mode */
        g_mesh_handler_data.bearer_tx_state.bearerPromiscuous = 
                    LE_BEARER_ACTIVE | GATT_SERVER_BEARER_ACTIVE;
        AppUpdateBearerState(&g_mesh_handler_data.bearer_tx_state);

        /* Save the state to NVM */
        Nvm_Write((uint16 *)&g_mesh_handler_data.bearer_tx_state, 
                  sizeof(CSR_MESH_BEARER_STATE_DATA_T),
                  NVM_OFFSET_BEARER_STATE);

        g_mesh_handler_data.assoc_state = app_state_not_associated;

        /* Write association state to NVM */
        Nvm_Write((uint16 *)&g_mesh_handler_data.assoc_state,
                 sizeof(g_mesh_handler_data.assoc_state),
                 NVM_OFFSET_ASSOCIATION_STATE);

        g_mesh_handler_data.ttl_value = DEFAULT_TTL_VALUE;
        /* Save the ttl value onto NVM */
        Nvm_Write((uint16 *)&g_mesh_handler_data.ttl_value, 
                  sizeof(g_mesh_handler_data.ttl_value),
                  NVM_OFFSET_TTL_VALUE);

        /* Reset the supported model groups and save it to NVM */
        initializeSupportedModelGroups();
        writeSupportedModelGroupsOntoNVM();

#ifdef ENABLE_BEACON_MODEL
        BeaconModelDataInit(NULL);
        WriteBeaconModelDataOntoNVM();
#endif

#ifdef ENABLE_BEACON_PROXY_MODEL
        BeaconProxyModelDataInit(NULL);
        WriteBeaconProxyModelDataOntoNVM();
#endif

#ifdef ENABLE_TIME_MODEL
        TimeModelDataInit(NULL);
        WriteTimeModelDataOntoNVM(NVM_OFFSET_TIME_INTERVAL);
#endif

#ifdef ENABLE_ACTION_MODEL
        ActionModelDataInit();
        WriteActionModelDataOntoNVM(NVM_OFFSET_ACTION_MODEL_DATA);
#endif
        /* Association is removed. Initiate association */
        InitiateAssociation();

        /* Update the TTL value onto mesh stack */
        CSRmeshSetDefaultTTL(g_mesh_handler_data.ttl_value);
    }
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      SetScanDutyCycle
 *
 *  DESCRIPTION
 *      This function sets the passed scan duty cycle on the mesh scheduler
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
extern void SetScanDutyCycle(uint8 scan_duty_cycle)
{
    le_params.generic_le_param.scan_param.scan_duty_param.scan_duty_cycle 
                                                              = scan_duty_cycle;
    le_params.generic_le_param.scan_param.scan_duty_param.min_scan_slot = 4;
    le_params.generic_le_param.scan_param_type = CSR_SCHED_SCAN_DUTY_PARAM;
    CSRSchedSetConfigParams(&le_params);
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      RestoreLightState
 *
 *  DESCRIPTION
 *      This function is called when the application wants to restore the light
 *      state based on association state.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void RestoreLightState(void)
{
    if(g_mesh_handler_data.assoc_state == app_state_associated)
    {
        IOTLightControlDevicePower(FALSE);
    }
    else if(g_mesh_handler_data.assoc_state == app_state_association_started)
    {
        /* Blink Light in Yellow to indicate association in progress */
        IOTLightControlDeviceBlink(127, 127, 0, 32, 32);
    }
    else
    {
        /* Restart Blue blink to indicate ready for association */
        IOTLightControlDeviceBlink(0, 0, 127, 32, 32);
    }
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      AppHandleAssociationComplete
 *
 *  DESCRIPTION
 *      This function is called on association complete.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void AppHandleAssociationComplete(void)
{
    /* Restore default light state */
    RestoreLightState();
}
#ifdef ENABLE_TIME_MODEL
/*-----------------------------------------------------------------------------*
 *
 *  NAME
 *      AppHandleCurrentTimeUpdate
 *
 *  DESCRIPTION
 *      This function is called when the current time is updated in the device.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *
 *----------------------------------------------------------------------------*/
extern void AppHandleCurrentTimeUpdate(uint8 current_time[])
{
#ifdef ENABLE_ACTION_MODEL
    /* Take actions to update the current time in the device */
    ActionModelSyncCurrentTime(current_time);
#endif
}
#endif

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

    Nvm_Write(&app_nvm_version, 
              sizeof(app_nvm_version),
              NVM_OFFSET_APP_NVM_VERSION);

    /* Store the Association State */
    Nvm_Write((uint16 *)&g_mesh_handler_data.assoc_state,
             sizeof(g_mesh_handler_data.assoc_state),
              NVM_OFFSET_ASSOCIATION_STATE);

    /* Save the bearer state to NVM */
    Nvm_Write((uint16 *)&g_mesh_handler_data.bearer_tx_state, 
              sizeof(CSR_MESH_BEARER_STATE_DATA_T),
              NVM_OFFSET_BEARER_STATE);

    /* Save the ttl value onto NVM */
    Nvm_Write((uint16 *)&g_mesh_handler_data.ttl_value, 
              sizeof(g_mesh_handler_data.ttl_value),
              NVM_OFFSET_TTL_VALUE);

    writeSupportedModelGroupsOntoNVM();

#ifdef ENABLE_BEACON_MODEL
    WriteBeaconModelDataOntoNVM();
#endif

#ifdef ENABLE_BEACON_PROXY_MODEL
    WriteBeaconProxyModelDataOntoNVM();
#endif

#ifdef ENABLE_TIME_MODEL
    WriteTimeModelDataOntoNVM(NVM_OFFSET_TIME_INTERVAL);
#endif

#ifdef ENABLE_ACTION_MODEL
    WriteActionModelDataOntoNVM(NVM_OFFSET_ACTION_MODEL_DATA);
#endif
}
#endif /* NVM_TYPE_FLASH */

