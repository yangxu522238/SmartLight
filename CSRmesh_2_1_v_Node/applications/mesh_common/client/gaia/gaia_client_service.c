/******************************************************************************
 *  %%fullcopyright(2016)
 *  %%version
 *  %%appversion
 *
 *  FILE
 *      gaia_client_service.c
 *
 *  DESCRIPTION
 *      This file contains the GAIA Client Service implementation.
 *
 ******************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/
#include <types.h>
#include <gatt.h>
#include <gatt_uuid.h>
#include <bt_event_types.h>
#include <mem.h>
#include <buf_utils.h>
#include <timer.h>

/*============================================================================*
 *  Local Header File
 *============================================================================*/
#include "cm_types.h"
#include "cm_api.h"
#include "nvm_access.h"
#include "gaia_uuids.h"
#include "gaia_client_service.h"
#include "gaia_otau_client_private.h"
#include "gaia_otau_client_api.h"
#include "gaia_client.h"

/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Characteristics Indexes */
typedef enum
{
    COMMAND_CHAR = 0,
    RESPONSE_CHAR,
    DATA_CHAR,
    GAIA_NUM_CHARACTERS     /* Number of characters defined in the GAIA client service */
} GAIA_CHARACTERISTICS_T;

/* Client Config Descriptor Indexes */
typedef enum
{
    RESPONSE_CCD = 0,
    GAIA_NUM_DESCRIPTORS   /* Maximum number of descriptors */
} GAIA_DESCRIPTORS_T;

/* Number of Instances of GAIA Client Service */
#define GAIA_CLIENT_SERVICE_INSTANCES       (1)

/*============================================================================*
 *  Private Data Type
 *============================================================================*/

/* Client instance */
typedef struct
{
    bond_handle_id      bond_id;
    uint16              start_handle;
    uint16              end_handle;
    uint16              cmd_char_handle;
    uint16              rsp_char_handle;
    uint16              data_char_handle;
    uint16              rsp_ccd_handle;
}GAIA_CLIENT_INSTANCE_T;

/* Client NVM Data */
typedef struct
{
    GAIA_CLIENT_INSTANCE_T    instance[GAIA_CLIENT_SERVICE_INSTANCES];
}GAIA_CLIENT_NVM_DATA_T;


/* Gaia Client Data */
typedef struct
{
   /* Current Configuration state */
   uint16           cur_state;

    /* The connection manager status for GAIA write status */
    cm_status_code       gaia_write_status;

    /* The Pending write data stored to be sent to FW on connection busy status */
    uint8                pending_data[20];

    /* The Pending write data length */
    uint16               pending_length;

}GAIA_CLIENT_DATA_T;

#define GAIA_CLIENT_SERVICE_VERSION             (0x0001)
#define GAIA_CLIENT_SERVICE_DATA_LEN            (sizeof(GAIA_CLIENT_NVM_DATA_T))


/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/

/* This function initialises descriptor data */
static void initialiseDescriptor(CM_DESCRIPTOR_T *descriptor);

/* This function initialises the characteristics data */
static void initialiseCharacteristics(CM_CHARACTERISTIC_T
                                      *characteristics, uint16 instance);

/* This function initialises the client service data */
static void gaiaClientDataInit(void);

/* Reads the client data from the NVM */
static void readDataFromNVM(bool nvm_start_fresh);

/* This function checks whether device is bonded to this service */
static int8 validBondId(bond_handle_id bond_id);

/* This function stores the GATT handles */
static void storeGattHandles(int8 instance_index);

/* This function handles the connection notification */
static void handleCMClientConnectionNotify(
                                        CM_CONNECTION_NOTIFY_T *p_event_data);

/* Functions handles the bonding status update notification */
static void handleCMClientBondingNotify(
                                CM_BONDING_NOTIFY_T *p_event_data);

/* This function handles the discovery complete */
static void handleCMClientDiscoveryComplete(
                                CM_DISCOVERY_COMPLETE_T *p_event_data);                                

/* This function handles the HR Measurement notification */
static void handleCMClientNotification(CM_NOTIFICATION_T *p_event_data);

/* This function handles the events from the connection manager */
static void handleConnMgrEvent (cm_event event_type,
                                     CM_EVENT_T *p_event_data);

/*============================================================================*
 *  Private Data
 *============================================================================*/

/* GAIA service uuid */
static uint16 g_gaia_client_service_uuid[] = {UUID_GAIA_SERVICE_1,
                                              UUID_GAIA_SERVICE_2,
                                              UUID_GAIA_SERVICE_3,
                                              UUID_GAIA_SERVICE_4,
                                              UUID_GAIA_SERVICE_5,
                                              UUID_GAIA_SERVICE_6,
                                              UUID_GAIA_SERVICE_7,
                                              UUID_GAIA_SERVICE_8};

/* GAIA command endpoint characteristic uuid */
static uint16 g_gaia_command_char_uuid[GAIA_CLIENT_SERVICE_INSTANCES][8];

/* GAIA response endpoint characteristic uuid */
static uint16 g_gaia_response_char_uuid[GAIA_CLIENT_SERVICE_INSTANCES][8];

/* GAIA data endpoint characteristic uuid */
static uint16 g_gaia_data_char_uuid[GAIA_CLIENT_SERVICE_INSTANCES][8];

/* GAIA descriptors */
static CM_DESCRIPTOR_T
        g_descriptors[GAIA_CLIENT_SERVICE_INSTANCES][GAIA_NUM_DESCRIPTORS];

/* GAIA characteristics */
static CM_CHARACTERISTIC_T
        g_characteristics[GAIA_CLIENT_SERVICE_INSTANCES][GAIA_NUM_CHARACTERS];

/* GAIA Service instance */
static CM_SERVICE_INSTANCE
        g_service_instances[GAIA_CLIENT_SERVICE_INSTANCES];

/* GAIA Client service */
static CM_SERVICE_T            g_gaia_client_service;

/* GAIA Client information */
static CM_CLIENT_INFO_T        g_gaia_client_service_info;

/* GAIA Client data */
static GAIA_CLIENT_DATA_T        g_gaia_client_data;

/* Handler */
static CM_HANDLERS_T g_gaia_client_handler =
{
    .pCallback = &handleConnMgrEvent
};

/*============================================================================*
 *  Private Function Implementation
 *============================================================================*/
/*----------------------------------------------------------------------------*
 *  NAME
 *      initialiseDescriptor
 *
 *  DESCRIPTION
 *      This function initialises descriptor data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void initialiseDescriptor(CM_DESCRIPTOR_T *descriptor)
{
    descriptor->uuid_type     = GATT_UUID16;
    descriptor->uuid          = UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION;
    descriptor->desc_handle   = CM_INVALID_ATT_HANDLE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      initialiseCharacteristics
 *
 *  DESCRIPTION
 *      This function initialises the characteristics data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void initialiseCharacteristics(CM_CHARACTERISTIC_T
                                      *characteristics, uint16 instance)
{
    uint16 index;
    for(index = 0; index < GAIA_NUM_CHARACTERS; index++)
    {
        characteristics[index].uuid_type    = GATT_UUID128;
        characteristics[index].value_handle = CM_INVALID_ATT_HANDLE;
        characteristics[index].properties   = 0;
        characteristics[index].nDescriptors = 0;
        characteristics[index].descriptors  = NULL;
    }

    characteristics[COMMAND_CHAR].uuid = &g_gaia_command_char_uuid[instance][0];
    characteristics[COMMAND_CHAR].uuid[0] = UUID_GAIA_COMMAND_ENDPOINT_1;
    characteristics[COMMAND_CHAR].uuid[1] = UUID_GAIA_COMMAND_ENDPOINT_2;
    characteristics[COMMAND_CHAR].uuid[2] = UUID_GAIA_COMMAND_ENDPOINT_3;
    characteristics[COMMAND_CHAR].uuid[3] = UUID_GAIA_COMMAND_ENDPOINT_4;
    characteristics[COMMAND_CHAR].uuid[4] = UUID_GAIA_COMMAND_ENDPOINT_5;
    characteristics[COMMAND_CHAR].uuid[5] = UUID_GAIA_COMMAND_ENDPOINT_6;
    characteristics[COMMAND_CHAR].uuid[6] = UUID_GAIA_COMMAND_ENDPOINT_7;
    characteristics[COMMAND_CHAR].uuid[7] = UUID_GAIA_COMMAND_ENDPOINT_8;

    characteristics[RESPONSE_CHAR].uuid = &g_gaia_response_char_uuid[instance][0];
    characteristics[RESPONSE_CHAR].uuid[0] = UUID_GAIA_RESPONSE_ENDPOINT_1;
    characteristics[RESPONSE_CHAR].uuid[1] = UUID_GAIA_RESPONSE_ENDPOINT_2;
    characteristics[RESPONSE_CHAR].uuid[2] = UUID_GAIA_RESPONSE_ENDPOINT_3;
    characteristics[RESPONSE_CHAR].uuid[3] = UUID_GAIA_RESPONSE_ENDPOINT_4;
    characteristics[RESPONSE_CHAR].uuid[4] = UUID_GAIA_RESPONSE_ENDPOINT_5;
    characteristics[RESPONSE_CHAR].uuid[5] = UUID_GAIA_RESPONSE_ENDPOINT_6;
    characteristics[RESPONSE_CHAR].uuid[6] = UUID_GAIA_RESPONSE_ENDPOINT_7;
    characteristics[RESPONSE_CHAR].uuid[7] = UUID_GAIA_RESPONSE_ENDPOINT_8;
    characteristics[RESPONSE_CHAR].nDescriptors = 1;
    characteristics[RESPONSE_CHAR].descriptors  = &g_descriptors[instance][0];
    initialiseDescriptor(characteristics[RESPONSE_CHAR].descriptors);

    characteristics[DATA_CHAR].uuid = &g_gaia_data_char_uuid[instance][0];
    characteristics[DATA_CHAR].uuid[0] = UUID_GAIA_DATA_ENDPOINT_1;
    characteristics[DATA_CHAR].uuid[1] = UUID_GAIA_DATA_ENDPOINT_2;
    characteristics[DATA_CHAR].uuid[2] = UUID_GAIA_DATA_ENDPOINT_3;
    characteristics[DATA_CHAR].uuid[3] = UUID_GAIA_DATA_ENDPOINT_4;
    characteristics[DATA_CHAR].uuid[4] = UUID_GAIA_DATA_ENDPOINT_5;
    characteristics[DATA_CHAR].uuid[5] = UUID_GAIA_DATA_ENDPOINT_6;
    characteristics[DATA_CHAR].uuid[6] = UUID_GAIA_DATA_ENDPOINT_7;
    characteristics[DATA_CHAR].uuid[7] = UUID_GAIA_DATA_ENDPOINT_8;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      gaiaClientDataInit
 *
 *  DESCRIPTION
 *      This function initialises the client service data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void gaiaClientDataInit(void)
{
    uint16 index;

    g_gaia_client_service.uuid_type = GATT_UUID128;
    g_gaia_client_service.uuid = g_gaia_client_service_uuid;
    g_gaia_client_service.mandatory = TRUE;
    g_gaia_client_service.nInstances = GAIA_CLIENT_SERVICE_INSTANCES;
    g_gaia_client_service.serviceInstances = g_service_instances;

    for(index = 0; index < GAIA_CLIENT_SERVICE_INSTANCES; index++)
    {
        g_service_instances[index].start_handle     =  CM_INVALID_ATT_HANDLE;
        g_service_instances[index].end_handle       =  CM_INVALID_ATT_HANDLE;
        g_service_instances[index].device_id        =  CM_INVALID_DEVICE_ID;
        g_service_instances[index].bond_id          =  CM_INVALID_BOND_ID;
        g_service_instances[index].nCharacteristics =  GAIA_NUM_CHARACTERS;
        g_service_instances[index].characteristics  = g_characteristics[index];

        initialiseCharacteristics(g_service_instances[index].characteristics,
                                  index);
    }

    /* Intialise the current state */
    g_gaia_client_data.cur_state = STATE_INIT;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      readDataFromNVM
 *
 *  DESCRIPTION
 *       Reads the client data from the NVM
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/

static void readDataFromNVM(bool nvm_start_fresh)
{
#ifndef NVM_DONT_PAD
    uint16 index;
    GAIA_CLIENT_NVM_DATA_T   nvm_data;

    /* If the service cannot read or repair the data in NVM
       we treat it as a fresh start */
    if (!nvm_start_fresh && Nvm_KeyValidate(GAIA_CLIENT_SERVICE_ID,
                                 GAIA_CLIENT_SERVICE_DATA_LEN,
                                 GAIA_CLIENT_SERVICE_VERSION,
                                 NULL) != sys_status_success)
    {
        nvm_start_fresh = TRUE;
    }

    if(nvm_start_fresh)
    {
        Nvm_KeyCreate(GAIA_CLIENT_SERVICE_ID,
                      GAIA_CLIENT_SERVICE_DATA_LEN,
                      GAIA_CLIENT_SERVICE_VERSION);
    }
    else
    {
        /* Read the NVM information */
        Nvm_KeyReadFull(GAIA_CLIENT_SERVICE_ID, nvm_data);
        
        for( index = 0; index < GAIA_CLIENT_SERVICE_INSTANCES; index++)
        {
            CM_SERVICE_INSTANCE *instance = &g_service_instances[index];
    
            GAIA_CLIENT_INSTANCE_T *nvm_instance = &nvm_data.instance[index];
    
            /* Get the bond id */
            instance->bond_id = nvm_instance->bond_id;
    
            if(instance->bond_id == CM_INVALID_BOND_ID)
                continue;
    
            instance->start_handle = nvm_instance->start_handle;
            instance->end_handle = nvm_instance->end_handle;
            instance->characteristics[COMMAND_CHAR].value_handle = nvm_instance->cmd_char_handle;
            instance->characteristics[RESPONSE_CHAR].value_handle = nvm_instance->rsp_char_handle;
            instance->characteristics[DATA_CHAR].value_handle = nvm_instance->data_char_handle;
            instance->characteristics[RESPONSE_CHAR].descriptors[RESPONSE_CCD].desc_handle = nvm_instance->rsp_ccd_handle;
        }
    }
#endif
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      validBondId
 *
 *  DESCRIPTION
 *      This function checks whether device is bonded to this service
 *
 *  RETURNS/MODIFIES
 *      int8 - index.
 *
 *---------------------------------------------------------------------------*/

static int8 validBondId(bond_handle_id bond_id)
{
    int8 index;
    for(index = 0; index < GAIA_CLIENT_SERVICE_INSTANCES; index++)
    {
        if(g_service_instances[index].bond_id == bond_id)
        {
            return index;
        }
    }
    return -1;
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      storeGattHandles
 *
 *  DESCRIPTION
 *      This function stores the GATT handles
 *
 *  RETURNS/MODIFIES
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

static void storeGattHandles(int8 instance_index)
{
#ifndef NVM_DONT_PAD
    CM_SERVICE_INSTANCE *instance = &g_service_instances[instance_index];
    GAIA_CLIENT_INSTANCE_T nvm_instance;

    nvm_instance.bond_id = instance->bond_id;
    nvm_instance.start_handle = instance->start_handle;
    nvm_instance.end_handle = instance->end_handle;
    nvm_instance.cmd_char_handle = instance->characteristics[COMMAND_CHAR].value_handle;
    nvm_instance.rsp_char_handle = instance->characteristics[RESPONSE_CHAR].value_handle;
    nvm_instance.data_char_handle = instance->characteristics[DATA_CHAR].value_handle;
    nvm_instance.rsp_ccd_handle = instance->characteristics[RESPONSE_CHAR].descriptors[RESPONSE_CCD].desc_handle;
    
    /* Write to NVM */
    Nvm_KeyWritePartial(GAIA_CLIENT_SERVICE_ID,
                        nvm_instance,
                        offsetof(GAIA_CLIENT_NVM_DATA_T, instance[instance_index]));
#endif
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      configureResponseConfig
 *
 *  DESCRIPTION
 *      This function configures the GAIA Response Endpoint CCD by enabling 
 *      notification on the GAIA Response Endpoint.
 *
 *  RETURNS
 *      TRUE if the initiation of the configuration procedure was successful
 *      FALSE if the remote device does not support the configuration 
 *      or configuration failed
 *----------------------------------------------------------------------------*/
static bool configureResponseConfig(device_handle_id device_id)
{
   int8 instance_index = CMClientFindDevice(&g_gaia_client_service, device_id);
   if(instance_index >= 0)
   {
        CM_SERVICE_INSTANCE *instance = &g_service_instances[instance_index];
        uint16 handle = instance->characteristics[RESPONSE_CHAR].
                        descriptors[RESPONSE_CCD].desc_handle;

        if(handle != CM_INVALID_ATT_HANDLE)
        {
            uint8 val[2], *p_val;
            p_val = val;
            BufWriteUint16(&p_val, gatt_client_config_notification);
            if(CMClientWriteRequest(device_id,
                                 GATT_WRITE_REQUEST,
                                 handle,
                                 2,
                                 val) == cm_status_success)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleCMClientConnectionNotify
 *
 *  DESCRIPTION
 *      This function handles the connection notify message received from CM.
 *
 *  RETURNS
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void handleCMClientConnectionNotify(CM_CONNECTION_NOTIFY_T *p_event_data)
{
    if(CMGetPeerDeviceRole(p_event_data->device_id) == con_role_peripheral)
    {
        if(p_event_data->result == cm_conn_res_success)
        {
            /* Get the bond from the device id */
            bond_handle_id bond_id =
                    CMGetBondIdFromDeviceId(p_event_data->device_id);

            if(bond_id != CM_INVALID_BOND_ID)
            {
                /* Check if the device belong to this service */
                int8 instance_index = validBondId(bond_id);
                if((instance_index >= 0))
                {
                    /* During reconnection, just add the device to correct
                     * instance as it's already bonded
                     */
                    g_service_instances[instance_index].device_id =
                            p_event_data->device_id;
                    
                    g_gaia_client_data.cur_state = STATE_GAIA_CONFIGURED;
                }
                else
                {
                    /* Since the server has not been bonded, we will
                       need to start the configuration procedure */
                    g_gaia_client_data.cur_state = STATE_INIT;
                }
            }
            else
            {
                /* Since the server has not been bonded, we will
                 * need to start the configuration procedure 
                 */
               g_gaia_client_data.cur_state = STATE_INIT;
            }

        }
        else if(p_event_data->result == cm_disconn_res_success)
        {           
            /* Get the service instance index */
            int8 instance_index =
                    CMClientFindDevice(&g_gaia_client_service,
                                       p_event_data->device_id);
            if(instance_index >= 0)
            {
                /* On disconnection remove the device
                 * from the service instance
                 */
                g_service_instances[instance_index].device_id = CM_INVALID_DEVICE_ID;
            }
        }
    }
    
    /* Notify the GAIA OTAU client of the connection */
     GaiaOtauClientConnNotifyEvent(p_event_data);

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleCMClientBondingNotify
 *
 *  DESCRIPTION
 *      This function handles the bonding notify message received from CM.
 *
 *  RETURNS
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void handleCMClientBondingNotify(CM_BONDING_NOTIFY_T *p_event_data)
{
    if(p_event_data->result == cm_bond_res_success)
    {
        /* Get the instance index */
        int8 instance_index = CMClientFindDevice(&g_gaia_client_service,
                                           p_event_data->device_id);

        /* Check if the bonded device belongs this service */
        if(instance_index >= 0)
        {
            /* Save the bond id */
            g_service_instances[instance_index].bond_id = p_event_data->bond_id;

            /* Store Handles */
            storeGattHandles(instance_index);
        }
    }
    else if(p_event_data->result == cm_unbond_res_success)
    {
#ifndef NVM_DONT_PAD
        int8 instance_index = validBondId(p_event_data->bond_id);

        /* Check if the unbonded device belongs to this service */
        if(instance_index >= 0)
        {
            g_service_instances[instance_index].bond_id = CM_INVALID_BOND_ID;

            /* Write to NVM */
            Nvm_KeyWritePartial(GAIA_CLIENT_SERVICE_ID,
                                g_service_instances[instance_index].bond_id,
                                offsetof(GAIA_CLIENT_NVM_DATA_T, instance[instance_index].bond_id));
        }
#endif
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleCMClientDiscoveryComplete
 *
 *  DESCRIPTION
 *       This function handles the discovery complete event
 *
 *  RETURNS
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void handleCMClientDiscoveryComplete(
                                CM_DISCOVERY_COMPLETE_T *p_event_data)
{
    if(p_event_data->status == cm_status_success)
    {
        int8 instance_index = CMClientFindDevice(&g_gaia_client_service,
                                         p_event_data->device_id);
        if(instance_index >= 0)
        {
            bond_handle_id bond_id =
                    CMGetBondIdFromDeviceId(p_event_data->device_id);
            if(bond_id != CM_INVALID_BOND_ID)
            {
                g_service_instances[instance_index].bond_id = bond_id;
    
                /* Write Handles to NVM */
                storeGattHandles(instance_index);
            }
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleCMClientNotification
 *
 *  DESCRIPTION
 *      This function handles the notification message received from CM.
 *
 *  RETURNS
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void handleCMClientNotification(CM_NOTIFICATION_T *p_event_data)
{
    CM_SERVICE_INSTANCE *instance =
            &g_service_instances[p_event_data->instance];

    if(p_event_data->handle ==
       instance->characteristics[RESPONSE_CHAR].value_handle)
    {
        /*  Parse the GAIA Notification */
        GAIAClientProcessCommand(p_event_data->device_id, 
                                 p_event_data->length,
                                 p_event_data->data);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleCMClientWriteCfm
 *
 *  DESCRIPTION
 *      This function handles the write confirm message received from CM.
 *
 *  RETURNS
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void handleCMClientWriteCfm(CM_WRITE_CFM_T *p_event_data)
{
    CM_SERVICE_INSTANCE *instance =
            &g_service_instances[p_event_data->instance];

    if(p_event_data->handle ==
           instance->characteristics[RESPONSE_CHAR].
                        descriptors[RESPONSE_CCD].desc_handle)
    {
        /* Request came from the state machine as opposed to
         * the user's application code 
         */
        GaiaConfigureService(p_event_data->device_id);

    }
    else if(p_event_data->handle ==
           instance->characteristics[COMMAND_CHAR].value_handle)
    {
        /* On receiving the write cfm check whether there is a pending packet
         * which needs to be retried because of cm status busy, if so resend 
         * the packet again.
         */
        if(g_gaia_client_data.gaia_write_status == cm_status_busy)
        {
            GaiaWriteCommandEndpoint(p_event_data->device_id,
                                     g_gaia_client_data.pending_length,
                                     g_gaia_client_data.pending_data);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleConnMgrEvent
 *
 *  DESCRIPTION
 *      This function handles the events from the connection manager.
 *
 *  RETURNS
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void handleConnMgrEvent(cm_event event_type,
                                     CM_EVENT_T *p_event_data)
{
    switch(event_type)
    {
        case CM_CONNECTION_NOTIFY:
            handleCMClientConnectionNotify(
                            (CM_CONNECTION_NOTIFY_T *)p_event_data);

        case CM_BONDING_NOTIFY:
            handleCMClientBondingNotify(
                            (CM_BONDING_NOTIFY_T *)p_event_data);
        break;

        case CM_DISCOVERY_COMPLETE:
            handleCMClientDiscoveryComplete(
                            (CM_DISCOVERY_COMPLETE_T *)p_event_data);
        break;

        case CM_WRITE_CFM:
            handleCMClientWriteCfm(
                            (CM_WRITE_CFM_T *)p_event_data);
        break;

        case CM_NOTIFICATION:
            handleCMClientNotification(
                            (CM_NOTIFICATION_T *)p_event_data);
        break;

        default:
        break;
    }
}

/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaInitClientService
 *
 *  DESCRIPTION
 *      This function intializes the Gaia Client Service. This function
 *      should be called during the application initialization.
 *
 *  RETURNS
 *      None
 *----------------------------------------------------------------------------*/
extern void GaiaInitClientService(bool nvm_start_fresh)
{
    gaiaClientDataInit();

#ifdef GAIA_OTAU_CLIENT_SUPPORT
    /* GAIA client support for the Over The Air Upgrade support */
    GaiaOtauClientInitEvent();
#endif /* GAIA_OTAU_CLIENT_SUPPORT */

    /* Load the Gaia Client data from NVM */
    readDataFromNVM(nvm_start_fresh);
    
    /* Register the callback handler with CM with the callback function handler
       and the client structure to be filled during discovery procedure
     */

    g_gaia_client_service_info.client_handler = g_gaia_client_handler;
    g_gaia_client_service_info.service_data = g_gaia_client_service;
    CMClientInitRegisterHandler(&g_gaia_client_service_info);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaClientIsServiceDiscovered
 *
 *  DESCRIPTION
 *      This function checks if the GAIA service handles are discovered
 *
 *  RETURNS
 *     Nothing
 *
 *----------------------------------------------------------------------------*/
extern bool GaiaClientIsServiceDiscovered(device_handle_id device_id)
{
    if(CMClientFindDevice(&g_gaia_client_service, device_id) == -1)
    {
        return FALSE;
    }
    return TRUE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaWriteCommandEndpoint
 *
 *  DESCRIPTION
 *      This function writes the GAIA command endpoint
 *
 *  RETURNS
 *      TRUE if the initiation of the write procedure was successful
 *      FALSE if the remote device does not support the configuration
 *----------------------------------------------------------------------------*/
extern bool GaiaWriteCommandEndpoint(device_handle_id device_id, uint16 len,
                                     uint8* value)
{
    int8 instance_index = CMClientFindDevice(&g_gaia_client_service, 
                                             device_id);

    if(instance_index >= 0)
    {
        CM_SERVICE_INSTANCE *instance = &g_service_instances[instance_index];
        uint16 handle = instance->characteristics[COMMAND_CHAR].value_handle;

        if(handle != CM_INVALID_ATT_HANDLE)
        {
            if(CMClientWriteRequest(device_id,
                                     GATT_WRITE_REQUEST,
                                     handle,
                                     len,
                                     value) == cm_status_success)
            {
                g_gaia_client_data.gaia_write_status = cm_status_success;
                return TRUE;
            }
            else
            {
                g_gaia_client_data.gaia_write_status = cm_status_busy;
                MemCopy(g_gaia_client_data.pending_data,value,len);
                g_gaia_client_data.pending_length = len;
            }
        }
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaReadResponseEndpoint
 *
 *  DESCRIPTION
 *      This function reads the GAIA response endpoint characteristic
 *
 *  RETURNS
 *      TRUE if the initiation of the read procedure was successful
 *      FALSE if the remote device does not support the configuration
 *----------------------------------------------------------------------------*/
extern bool GaiaReadResponseEndpoint(device_handle_id device_id)
{
   int8 instance_index = CMClientFindDevice(&g_gaia_client_service, device_id);
   if(instance_index >= 0)
   {
        CM_SERVICE_INSTANCE *instance = &g_service_instances[instance_index];
        uint16 handle = instance->characteristics[RESPONSE_CHAR].value_handle;

        if(handle != CM_INVALID_ATT_HANDLE)
        {
            CMClientReadRequest(device_id, handle);

            return TRUE;
        }
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaConfigureService
 *
 *  DESCRIPTION
 *      This function runs a state machine which configures the Gaia service
 *      The function enables notifications for the response endpoint characteristic
 *      on the server.
 *
 *  RETURNS
 *     Nothing
 *
 *----------------------------------------------------------------------------*/
extern void GaiaConfigureService(device_handle_id device_id)
{
    switch(g_gaia_client_data.cur_state)
    {
        case STATE_INIT:
        {
            g_gaia_client_data.cur_state = STATE_GAIA_CONFIG_RESPONSE_CHAR;
            if(!configureResponseConfig(device_id))
            {
                g_gaia_client_data.cur_state = STATE_GAIA_CONFIG_FAILED;
            }
        }
        break;

        case STATE_GAIA_CONFIG_RESPONSE_CHAR:
        case STATE_GAIA_CONFIGURED:
        {
            g_gaia_client_data.cur_state = STATE_GAIA_CONFIGURED;
        }
        break;

        default:
        break;
    }
    /* Notify the configuration status to the application */
    GaiaOtauClientConfigurationComplete(device_id,g_gaia_client_data.cur_state);
}



