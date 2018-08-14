/******************************************************************************
 *  Copyright 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  CSRmesh is a product of Qualcomm Technologies International Ltd.
 *
 *  FILE
 *      app_otau_client_handler.c
 *
 *  DESCRIPTION
 *      Application OTAU Upgrade Client implementation.
 *
 *****************************************************************************/
#ifdef GAIA_OTAU_RELAY_SUPPORT

/*=============================================================================*
 *  SDK Header Files
 *============================================================================*/
#include <gatt.h>
#include <gatt_prim.h>
#include <battery.h>
#include <buf_utils.h>
#include <mem.h>
#include <timer.h>
#include <storage.h>
#include <store_update.h>
#include <configstore_id.h>
#include <configstore_api.h>
#include <store_update_msg.h>

/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "gaia_client.h"
#include "gaia.h"
#include "byte_utils.h"
#include "gaia_otau_private.h"
#include "gaia_otau_client_api.h"
#include "app_otau_client_handler.h"
#include "gaia_client_service.h"
#include "cm_types.h"
#include "cm_api.h"
#include "nvm_access.h"
#include "connection_handler.h"
#include "app_debug.h"
#include "largeobjecttransfer_model_handler.h"
#include "app_mesh_handler.h"

/*=============================================================================*
 *  Private Definitions
 *============================================================================*/

#define MAX_DATA_BYTES_SENT                            (12) // (20 -8)
#define CONN_INTERVAL                                  (2000000)
/* NVM Offsets for Upgrade Client Library */
#define NVM_PREV_HEADER_BODY_OFFSET          0
#define NVM_PREV_PARTITION_INFO_OFFSET       MESH_HEADER_BODY_SIZE
#define NVM_PREV_FOOTER_SIGNATURE_OFFSET     NVM_PREV_PARTITION_INFO_OFFSET + \
                                             PARTITION_INFO_SIZE
#define NVM_CUR_HEADER_BODY_OFFSET           NVM_PREV_FOOTER_SIGNATURE_OFFSET +\
                                             PARTITION_FOOTER_SIGNATURE_LENGTH
#define NVM_CUR_PARTITION_INFO_OFFSET        NVM_CUR_HEADER_BODY_OFFSET + MESH_HEADER_BODY_SIZE
#define NVM_CUR_FOOTER_SIGNATURE_OFFSET      NVM_CUR_PARTITION_INFO_OFFSET + \
                                             PARTITION_INFO_SIZE
                                             
#define NVM_OTAU_PREV_STORE_ID_OFFSET        NVM_CUR_FOOTER_SIGNATURE_OFFSET +\
                                             PARTITION_FOOTER_SIGNATURE_LENGTH
#define NVM_OTAU_PREV_STORE_TYPE_OFFSET      NVM_OTAU_PREV_STORE_ID_OFFSET + sizeof(uint16)
#define NVM_OTAU_PREV_PARTITION_SIZE_OFFSET  NVM_OTAU_PREV_STORE_TYPE_OFFSET + sizeof(uint16)
                                             
#define NVM_OTAU_CUR_STORE_ID_OFFSET         NVM_OTAU_PREV_PARTITION_SIZE_OFFSET +\
                                             sizeof(appOtaData.prev_partition_data_length)
#define NVM_OTAU_CUR_STORE_TYPE_OFFSET       NVM_OTAU_CUR_STORE_ID_OFFSET + sizeof(uint16)
#define NVM_OTAU_CUR_PARTITION_SIZE_OFFSET   NVM_OTAU_CUR_STORE_TYPE_OFFSET + sizeof(uint16)
#define NVM_OTAU_COMMIT_DONE_OFFSET          NVM_OTAU_CUR_PARTITION_SIZE_OFFSET + sizeof(appOtaData.cur_partition_data_length)
#define NVM_OTAU_CALCULATE_HASH_OFFSET       NVM_OTAU_COMMIT_DONE_OFFSET  + sizeof(appOtaData.commit_done)
#define GAIA_OTAU_CLIENT_SERVICE_NVM_MEMORY_WORDS    NVM_OTAU_CALCULATE_HASH_OFFSET  +\
                                                      sizeof(appOtaData.calculate_hash)

/*============================================================================*
 *  Private Data Types
 *============================================================================*/

/* This structure defines the local data store for this module */
typedef struct {
    memory_address_t prev_partition_data_length; 
    memory_address_t cur_partition_data_length; 
    uint16 nvm_offset;
    uint32 app_current_offset;
    uint32 header_current_offset;
    uint32 footer_current_offset;
    uint8 prev_header[HEADER_SIZE];
    uint8 prev_footer[FOOTER_SIZE];
    uint8 cur_header[HEADER_SIZE];
    uint8 cur_footer[FOOTER_SIZE];
    uint16 cur_store_id;
    uint16 cur_store_type;
    uint16 prev_store_id;
    uint16 prev_store_type;
    struct {
        uint16 sha256_sum[SHA256_SIZE_WORDS];       /* sum of all the SHA256s */
        uint16 sha256_buffer[SHA256_SIZE_WORDS];    /* SHA256 that is calculated from the Flash */
        uint32 length;                      /* expected OEM signature length */
    } oem_signature;
    timer_id announce_tid;
    store_id_t cs_id;
    store_id_t app_id;
    timer_id start_upgrade_tid;
    device_handle_id device_id;
    uint8 announce_count;
    bool commit_done;
    bool calculate_hash;
    uint8 footer_signature[PARTITION_FOOTER_SIGNATURE_LENGTH];
    bool store_hash_request_sent;
    handle_t store_handle;
    bool app_connected;
} APP_OTA_CLIENT_LOCAL_DATA_T;

/*=============================================================================*
 *  Private Data
 *============================================================================*/

/* This local data store for this module */
static APP_OTA_CLIENT_LOCAL_DATA_T appOtaData;

/* LOT Announce data */
LARGEOBJECTTRANSFER_ANNOUNCE_T lot_announce_data;

/*=============================================================================*
 *  Private Function Implementations
 *============================================================================*/
/*----------------------------------------------------------------------------*
 *  NAME
 *      resetOtauTransferStateData
 *
 *  DESCRIPTION
 *      This function resets the State data used for new upgrade
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void resetOtauTransferStateData(void)
{
    appOtaData.app_current_offset = 0;
    appOtaData.footer_current_offset = 0;
    appOtaData.header_current_offset = 0;
    appOtaData.announce_count = 0;
}

/*-----------------------------------------------------------------------------
 *  NAME
 *      readDataFromNVM
 *
 *  DESCRIPTION
 *      This function is used to read GAIA client service specific data store in NVM
 *
 *  RETURNS:
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void readDataFromNVM(bool nvm_start_fresh, uint16 *nvm_offset)
{
    appOtaData.nvm_offset = *nvm_offset;

    if(!nvm_start_fresh)
    {
         /* Read GAIA OTAu Header */
         Nvm_Read((uint16*)&appOtaData.prev_header[PARTITION_HEADER_BODY_OFFSET],
                 MESH_HEADER_BODY_SIZE,
                 appOtaData.nvm_offset + NVM_PREV_HEADER_BODY_OFFSET);
        
         Nvm_Read((uint16*)&appOtaData.prev_header[PARTITION_LENGTH_OFFSET],
              PARTITION_INFO_SIZE,
              appOtaData.nvm_offset + NVM_PREV_PARTITION_INFO_OFFSET);
        
         Nvm_Read((uint16*)&appOtaData.prev_footer[PARTITION_FOOTER_SIGNATURE_OFFSET],
                 PARTITION_FOOTER_SIGNATURE_LENGTH,
                 appOtaData.nvm_offset + NVM_PREV_FOOTER_SIGNATURE_OFFSET);
         
         Nvm_Read((uint16*)&appOtaData.cur_header[PARTITION_HEADER_BODY_OFFSET],
                 MESH_HEADER_BODY_SIZE,
                 appOtaData.nvm_offset + NVM_CUR_HEADER_BODY_OFFSET);
        
         Nvm_Read((uint16*)&appOtaData.cur_header[PARTITION_LENGTH_OFFSET],
              PARTITION_INFO_SIZE,
              appOtaData.nvm_offset + NVM_CUR_PARTITION_INFO_OFFSET);
        
         Nvm_Read((uint16*)&appOtaData.cur_footer[PARTITION_FOOTER_SIGNATURE_OFFSET],
                 PARTITION_FOOTER_SIGNATURE_LENGTH,
                 appOtaData.nvm_offset + NVM_CUR_FOOTER_SIGNATURE_OFFSET);
         
         Nvm_Read((uint16*)&appOtaData.cur_store_id,
                  sizeof(uint16),
                 appOtaData.nvm_offset + NVM_OTAU_CUR_STORE_ID_OFFSET);
         
         Nvm_Read((uint16*)&appOtaData.cur_store_type,
                  sizeof(uint16),
                 appOtaData.nvm_offset + NVM_OTAU_CUR_STORE_TYPE_OFFSET);
        
         Nvm_Read((uint16*)&appOtaData.cur_partition_data_length,
              sizeof(appOtaData.cur_partition_data_length),
              appOtaData.nvm_offset + NVM_OTAU_CUR_PARTITION_SIZE_OFFSET);
         
         Nvm_Read((uint16*)&appOtaData.prev_store_id,
                  sizeof(uint16),
                 appOtaData.nvm_offset + NVM_OTAU_PREV_STORE_ID_OFFSET);
         
         Nvm_Read((uint16*)&appOtaData.prev_store_type,
                  sizeof(uint16),
                 appOtaData.nvm_offset + NVM_OTAU_PREV_STORE_TYPE_OFFSET);
        
         Nvm_Read((uint16*)&appOtaData.prev_partition_data_length,
              sizeof(appOtaData.prev_partition_data_length),
              appOtaData.nvm_offset + NVM_OTAU_PREV_PARTITION_SIZE_OFFSET);
         
         Nvm_Read((uint16*)&appOtaData.commit_done,
              sizeof(appOtaData.commit_done),
              appOtaData.nvm_offset + NVM_OTAU_COMMIT_DONE_OFFSET);
         
         Nvm_Read((uint16*)&appOtaData.calculate_hash,
              sizeof(appOtaData.calculate_hash),
              appOtaData.nvm_offset + NVM_OTAU_CALCULATE_HASH_OFFSET);
    }
    else
    {
        /* Write GAIA OTAu InProgressIdentifier config */
        Nvm_Write((uint16*)&appOtaData.prev_header[PARTITION_HEADER_BODY_OFFSET],
                 MESH_HEADER_BODY_SIZE,
                 appOtaData.nvm_offset + NVM_PREV_HEADER_BODY_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.prev_header[PARTITION_LENGTH_OFFSET],
              PARTITION_INFO_SIZE,
              appOtaData.nvm_offset + NVM_PREV_PARTITION_INFO_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.prev_footer[PARTITION_FOOTER_SIGNATURE_OFFSET],
                 PARTITION_FOOTER_SIGNATURE_LENGTH,
                 appOtaData.nvm_offset + NVM_PREV_FOOTER_SIGNATURE_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.cur_header[PARTITION_HEADER_BODY_OFFSET],
                 MESH_HEADER_BODY_SIZE,
                 appOtaData.nvm_offset + NVM_CUR_HEADER_BODY_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.cur_header[PARTITION_LENGTH_OFFSET],
              PARTITION_INFO_SIZE,
              appOtaData.nvm_offset + NVM_CUR_PARTITION_INFO_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.cur_footer[PARTITION_FOOTER_SIGNATURE_OFFSET],
                 PARTITION_FOOTER_SIGNATURE_LENGTH,
                 appOtaData.nvm_offset + NVM_CUR_FOOTER_SIGNATURE_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.cur_store_id,
                  sizeof(uint16),
                 appOtaData.nvm_offset + NVM_OTAU_CUR_STORE_ID_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.cur_store_type,
                  sizeof(uint16),
                 appOtaData.nvm_offset + NVM_OTAU_CUR_STORE_TYPE_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.cur_partition_data_length,
              sizeof(appOtaData.cur_partition_data_length),
              appOtaData.nvm_offset + NVM_OTAU_CUR_PARTITION_SIZE_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.prev_store_id,
                  sizeof(uint16),
                 appOtaData.nvm_offset + NVM_OTAU_PREV_STORE_ID_OFFSET);
         
        Nvm_Write((uint16*)&appOtaData.prev_store_type,
                  sizeof(uint16),
                 appOtaData.nvm_offset + NVM_OTAU_PREV_STORE_TYPE_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.prev_partition_data_length,
              sizeof(appOtaData.prev_partition_data_length),
              appOtaData.nvm_offset + NVM_OTAU_PREV_PARTITION_SIZE_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.commit_done,
              sizeof(appOtaData.commit_done),
              appOtaData.nvm_offset + NVM_OTAU_COMMIT_DONE_OFFSET);
        
        Nvm_Write((uint16*)&appOtaData.calculate_hash,
              sizeof(appOtaData.calculate_hash),
              appOtaData.nvm_offset + NVM_OTAU_CALCULATE_HASH_OFFSET);
    }

    /* Increment the offset by the number of words of NVM memory required
     * by GAIA OTAu service
     */
    *nvm_offset += GAIA_OTAU_CLIENT_SERVICE_NVM_MEMORY_WORDS;
}

/*-----------------------------------------------------------------------------
 *  NAME
 *      updatePreviousStore
 *
 *  DESCRIPTION
 *      This function is used to update previour store
 *
 *  RETURNS:
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void updatePreviousStore(void)
{
    appOtaData.prev_partition_data_length = appOtaData.cur_partition_data_length;
    appOtaData.prev_store_type = appOtaData.cur_store_type;
    appOtaData.prev_store_id = appOtaData.cur_store_id;
    MemCopy(appOtaData.prev_header,appOtaData.cur_header,sizeof(appOtaData.prev_header));
        
    Nvm_Write((uint16*)&appOtaData.prev_store_id,
              sizeof(uint16),
              appOtaData.nvm_offset + NVM_OTAU_PREV_STORE_ID_OFFSET);
    
    Nvm_Write((uint16*)&appOtaData.prev_store_type,
              sizeof(uint16),
              appOtaData.nvm_offset + NVM_OTAU_PREV_STORE_TYPE_OFFSET);
    
    Nvm_Write((uint16*)&appOtaData.prev_partition_data_length,
              sizeof(appOtaData.prev_partition_data_length),
              appOtaData.nvm_offset + NVM_OTAU_PREV_PARTITION_SIZE_OFFSET);
    
    Nvm_Write((uint16*)&appOtaData.prev_header[PARTITION_HEADER_BODY_OFFSET],
              MESH_HEADER_BODY_SIZE,
              appOtaData.nvm_offset + NVM_PREV_HEADER_BODY_OFFSET);
    
    Nvm_Write((uint16*)&appOtaData.prev_header[PARTITION_LENGTH_OFFSET],
              PARTITION_INFO_SIZE,
              appOtaData.nvm_offset + NVM_PREV_PARTITION_INFO_OFFSET);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      startSHA256sum
 *
 *  DESCRIPTION
 *      Start a SHA256 calculation on the current partition
 *
 *  PARAMETERS
 *
 *  RETURNS
 *      None
 *
 *---------------------------------------------------------------------------*/
static void startSHA256sum(void)
{
    handle_t handle;
    uint8 i;
    
    for ( i = 0; i < SHA256_SIZE_WORDS; i++ )
    {
        appOtaData.oem_signature.sha256_sum[i] = 0;
    }

    appOtaData.store_hash_request_sent = TRUE;
    Storage_FindStore(appOtaData.cur_store_id, appOtaData.cur_store_type, &handle);
    StoreUpdate_HashStore(  handle,
                            /* header is not included */
                            STORE_HEADER_SIZE_WORDS,
                            /* convert from octets to 16bit words */
                            appOtaData.cur_partition_data_length / 2 - STORE_HEADER_SIZE_WORDS,
                            appOtaData.oem_signature.sha256_buffer);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      setCalculateHashStatus
 *
 *  DESCRIPTION
 *      This function sets the calculate hash status
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void setCalculateHashStatus(bool status)
{
    appOtaData.calculate_hash = status;
    Nvm_Write((uint16*)&appOtaData.calculate_hash,
              sizeof(appOtaData.calculate_hash),
              appOtaData.nvm_offset + NVM_OTAU_CALCULATE_HASH_OFFSET);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      startUpgradeTimerExpiry
 *
 *  DESCRIPTION
 *      This function handles the start upgrade timer expiry
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void startUpgradeTimerExpiry(timer_id tid)
{
    if(appOtaData.start_upgrade_tid == tid)
    {
        if(appOtaData.cur_store_id != 0)
        {
            GaiaOtauInProgress(TRUE);
            GaiaOtauClientStartUpgrade(appOtaData.device_id);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      gaiaClientDataTransferEvent
 *
 *  DESCRIPTION
 *      This function is called when an event is received to transfer the data
 *      onto the GAIA client from the application
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void gaiaClientDataTransferEvent(device_handle_id device_id, 
                                        uint32 req_num_bytes)
{
    uint8 upgrade_data[MAX_DATA_BYTES_SENT + 1];
    upgrade_data[0] = 0;
    bool reached_eof = FALSE;

    if ((appOtaData.header_current_offset + appOtaData.app_current_offset +
        appOtaData.footer_current_offset + req_num_bytes) <= 
       (appOtaData.cur_partition_data_length + HEADER_SIZE + FOOTER_SIZE))
    {
        GaiaClientSendAcknowledgement(appOtaData.device_id, GAIA_STATUS_SUCCESS);
    }
    else
    {
        GaiaClientSendAcknowledgement(appOtaData.device_id, 
                                      GAIA_STATUS_INVALID_PARAMETER);
        return;
    }

    if(appOtaData.header_current_offset < HEADER_SIZE)
    {
        uint32 remaining_header = HEADER_SIZE - appOtaData.header_current_offset;
        if(req_num_bytes <= remaining_header)
        {
            MemCopy(&upgrade_data[1],&appOtaData.cur_header[appOtaData.header_current_offset],req_num_bytes);
            appOtaData.header_current_offset += req_num_bytes;
            if(req_num_bytes == remaining_header)
                    Storage_FindStore(appOtaData.cur_store_id, appOtaData.cur_store_type, 
                    &appOtaData.store_handle);
         }
     }
    else if(appOtaData.app_current_offset < appOtaData.cur_partition_data_length)
        {
            uint32 remaining_app = appOtaData.cur_partition_data_length - appOtaData.app_current_offset;
            
            if(req_num_bytes <= remaining_app)
            {
                uint16 buffer[6];
                uint8 temp[12];
                uint16 num_bytes = (req_num_bytes/2);
                uint32 word_offset = (appOtaData.app_current_offset/2);
                Storage_BlockRead(appOtaData.store_handle,buffer,word_offset,&num_bytes);
                MemCopyUnPack(temp,buffer,req_num_bytes);
                SwapBytes(req_num_bytes, temp, &upgrade_data[1]);
                appOtaData.app_current_offset += req_num_bytes;
            }
        }
        else if(appOtaData.footer_current_offset < FOOTER_SIZE)
        {
            uint32 remaining_footer = FOOTER_SIZE - appOtaData.footer_current_offset;
            if(req_num_bytes <= remaining_footer)
            {
                MemCopy(&upgrade_data[1],&appOtaData.cur_footer[appOtaData.footer_current_offset],req_num_bytes);
                appOtaData.footer_current_offset += req_num_bytes;
                if(appOtaData.footer_current_offset == FOOTER_SIZE)
                {
                    reached_eof = TRUE;
                    upgrade_data[0] =  0x01;
                }
            } 
        }
        DEBUG_STR("\r\nData Sent");
        GaiaOtauClientSendData(device_id, upgrade_data, req_num_bytes + 1, reached_eof);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      gaiaClientConnNotifyEvent
 *
 *  DESCRIPTION
 *      This function is used to handle a connection event
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void gaiaClientConnNotifyEvent(device_handle_id device_id,
                                      cm_conn_result result)
{
    if(result == cm_conn_res_success)
    {
        appOtaData.app_connected = TRUE;
        TimerDelete(appOtaData.announce_tid);
        appOtaData.announce_tid = TIMER_INVALID;
    }
    else
    {
        appOtaData.app_connected = FALSE;
        TimerDelete(appOtaData.start_upgrade_tid);
        appOtaData.start_upgrade_tid = TIMER_INVALID;
        if(appOtaData.device_id == device_id)
        {
            appOtaData.device_id = CM_INVALID_DEVICE_ID;
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appHandleGaiaOtauClientEvent
 *
 *  DESCRIPTION
 *      This function handles the events from the GAIA OTAU Client library
 *
 *  PARAMETERS
 *      event[in]       The type of event being received
 *      *data[in,out]   Pointer to the event data associated with the event
 *
 *  RETURNS
 *     sys_status: sys_status_success if event handled, or sys_status_failed.
 *----------------------------------------------------------------------------*/
static sys_status appHandleGaiaOtauClientEvent(gaia_otau_client_event event, 
                                               GAIA_OTAU_CLIENT_EVENT_T *data)
{
    sys_status status = sys_status_success;
    
    switch (event)
    {
        case gaia_otau_client_config_status_event:
        {
            GAIA_OTAU_CLIENT_CONFIG_STATUS_EVENT_T* config_event = 
                                (GAIA_OTAU_CLIENT_CONFIG_STATUS_EVENT_T*)data;
            if(config_event->status == STATE_GAIA_CONFIGURED)
            {
                appOtaData.device_id = config_event->device_id;
                appOtaData.start_upgrade_tid = TimerCreate(CONN_INTERVAL,
                                                           TRUE, 
                                                           startUpgradeTimerExpiry);
            }
        }
        break;

        case gaia_otau_client_conn_event:
        {
            GAIA_OTAU_CLIENT_CONN_EVENT_T* conn_event = 
                                (GAIA_OTAU_CLIENT_CONN_EVENT_T*)data;

                GaiaOtauClientUpdateConnStatus(conn_event->device_id,
                                               conn_event->result);

                gaiaClientConnNotifyEvent(conn_event->device_id,
                                          conn_event->result);
        }
        break;

        case gaia_otau_client_data_req_event:
        {
            GAIA_OTAU_CLIENT_DATA_REQ_EVENT_T* data_req_event = 
                                (GAIA_OTAU_CLIENT_DATA_REQ_EVENT_T*)data;

            gaiaClientDataTransferEvent(data_req_event->device_id,
                                        data_req_event->no_bytes_requested);
        }
        break;

        case gaia_otau_client_reset_event:
            resetOtauTransferStateData();
        break;

        case gaia_otau_client_upgrade_end_event:
        {
            GAIA_OTAU_CLIENT_UPGRADE_END_EVENT_T* upg_end_event = 
                                (GAIA_OTAU_CLIENT_UPGRADE_END_EVENT_T*)data;

            DEBUG_STR("\r\nDisconnecting the device");
            CMDisconnect(upg_end_event->device_id);
        }
        break;

        default:
            break;
    }
    
    return status;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      handleAnnounceTimerExpiry
 *
 *  DESCRIPTION
 *      This function handles the announce timer expiry
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void handleAnnounceTimerExpiry(timer_id tid)
{
    if(tid == appOtaData.announce_tid)
    {
        SendLOTAnnouncePacket();
    }
}

/*=============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      AppOtauClientInit
 *
 *  DESCRIPTION
 *      This function is used to initialise upgrade client data structure.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void AppOtauClientInit(void)
{
    GaiaOtauClientRegisterCallback(appHandleGaiaOtauClientEvent);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaAppClientInit
 *
 *  DESCRIPTION
 *      This function is used to initialise application upgrade client data
 *      structure.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void GaiaAppClientInit(bool nvm_start_fresh, uint16 *nvm_offset)
{
    /* Initialise appOtaData */
    resetOtauTransferStateData();
    appOtaData.app_connected = FALSE;
    appOtaData.commit_done = TRUE;
    appOtaData.calculate_hash = FALSE;
    appOtaData.store_hash_request_sent = FALSE;
    appOtaData.prev_partition_data_length = 0;
    appOtaData.cur_partition_data_length = 0;
    appOtaData.prev_store_id = 0; 
    appOtaData.cur_store_id = 0;
    appOtaData.prev_store_id = 0; 
    appOtaData.cur_store_id = 0;
    appOtaData.prev_store_type = 0; 
    appOtaData.cur_store_type = 0;
    uint32 mesh_hdr_body_size = MESH_HEADER_BODY_SIZE;
    uint32 part_sig_size = PARTITION_FOOTER_SIGNATURE_LENGTH;
    TimerDelete(appOtaData.announce_tid);
    appOtaData.announce_tid = TIMER_INVALID;

    /* Intialise the timer */
    TimerDelete(appOtaData.start_upgrade_tid);
    appOtaData.start_upgrade_tid = TIMER_INVALID;

    /* Initialise Previous Store Header */
    MemCopy(appOtaData.prev_header,"APPUHDR4",PARTITION_HEADER_ID_SIZE);
    SetUint32InArray(appOtaData.prev_header, PARTITION_HEADER_LENGTH_OFFSET,mesh_hdr_body_size);
    MemSet(&appOtaData.prev_header[PARTITION_HEADER_BODY_OFFSET],0,MESH_HEADER_BODY_SIZE );
    MemCopy(&appOtaData.prev_header[PARTITION_ID_OFFSET],"PARTDATA",PARTITION_HEADER_ID_SIZE);    
    MemSet(&appOtaData.prev_header[PARTITION_LENGTH_OFFSET],0,PARTITION_INFO_SIZE);
    
    /* Initialise Previous Store Footer */
    MemCopy(&appOtaData.prev_footer,"APPUPFTR",PARTITION_FOOTER_ID_SIZE);
    SetUint32InArray(appOtaData.prev_footer, PARTITION_FOOTER_LENGTH_OFFSET,part_sig_size);
    MemSet(&appOtaData.prev_footer[PARTITION_FOOTER_SIGNATURE_OFFSET],0,PARTITION_FOOTER_SIGNATURE_LENGTH);
    
    /* Initialise Current Store Header */
    MemCopy(appOtaData.cur_header,"APPUHDR4",HEADER_ID_SIZE );
    SetUint32InArray(appOtaData.cur_header, PARTITION_HEADER_LENGTH_OFFSET,mesh_hdr_body_size);
    MemSet(&appOtaData.cur_header[PARTITION_HEADER_BODY_OFFSET],0,MESH_HEADER_BODY_SIZE );
    MemCopy(&appOtaData.cur_header[PARTITION_ID_OFFSET],"PARTDATA",PARTITION_HEADER_ID_SIZE);    
    MemSet(&appOtaData.cur_header[PARTITION_LENGTH_OFFSET],0,PARTITION_INFO_SIZE);    
    
    /* Initialise User Store Footer */
    MemCopy(&appOtaData.cur_footer,"APPUPFTR",PARTITION_FOOTER_ID_SIZE);
    SetUint32InArray(appOtaData.cur_footer, PARTITION_FOOTER_LENGTH_OFFSET,part_sig_size);
    MemSet(&appOtaData.cur_footer[PARTITION_FOOTER_SIGNATURE_OFFSET],0,PARTITION_FOOTER_SIGNATURE_LENGTH );
    
    /* Read data from NVM */
    readDataFromNVM(nvm_start_fresh, nvm_offset);
}

/*-----------------------------------------------------------------------------
 *  NAME
 *      StoreHeaderDataToNVM
 *
 *  DESCRIPTION
 *      This function is used to store header data in NVM
 *
 *  RETURNS:
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void StoreHeaderDataToNVM(uint8* p_hdr_data)
{
    MemCopy(&appOtaData.cur_header[PARTITION_HEADER_BODY_OFFSET],p_hdr_data,MESH_HEADER_BODY_SIZE);
    
    /* Write to NVM */
    Nvm_Write((uint16*)&appOtaData.cur_header[PARTITION_HEADER_BODY_OFFSET],
              MESH_HEADER_BODY_SIZE,
              appOtaData.nvm_offset + NVM_CUR_HEADER_BODY_OFFSET);
}

/*-----------------------------------------------------------------------------
 *  NAME
 *      StorePartitionDataToNVM
 *
 *  DESCRIPTION
 *      This function is used to store partition data info in NVM
 *
 *  RETURNS:
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void StorePartitionDataToNVM(GAIA_OTAU_EVENT_PARTITION_INFO_T* p_part_data)
{
    SetUint32InArray(appOtaData.cur_header,PARTITION_LENGTH_OFFSET,
                     (p_part_data->partition_size + 4));
    SetUint16InArray(appOtaData.cur_header,
                     PARTITION_LENGTH_OFFSET + 4,
                     p_part_data->partition_type);
    SetUint16InArray(appOtaData.cur_header,
                     PARTITION_LENGTH_OFFSET + 6,
                     p_part_data->partition_id);
    /* Write Partition information to NVM */
    Nvm_Write((uint16*)&appOtaData.cur_header[PARTITION_LENGTH_OFFSET],
              PARTITION_INFO_SIZE,
              appOtaData.nvm_offset + NVM_CUR_PARTITION_INFO_OFFSET);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      StoreRelayStoreInfo
 *
 *  DESCRIPTION
 *      This function is used to store the relay store information to NVM
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void StoreRelayStoreInfo(GAIA_OTAU_EVENT_PARTITION_INFO_T *p_store_info)
{
    appOtaData.cur_partition_data_length =  p_store_info->partition_size;
    appOtaData.cur_store_type = p_store_info->partition_type;
    appOtaData.cur_store_id = p_store_info->partition_id;
    
    Nvm_Write((uint16*)&appOtaData.cur_partition_data_length,
              sizeof(appOtaData.cur_partition_data_length),
              appOtaData.nvm_offset + NVM_OTAU_CUR_PARTITION_SIZE_OFFSET);
    Nvm_Write((uint16*)&appOtaData.cur_store_type,
                 sizeof(uint16),
                 appOtaData.nvm_offset + NVM_OTAU_CUR_STORE_TYPE_OFFSET);
    Nvm_Write((uint16*)&appOtaData.cur_store_id,
                 sizeof(uint16),
                 appOtaData.nvm_offset + NVM_OTAU_CUR_STORE_ID_OFFSET);
    
}

/*-----------------------------------------------------------------------------
 *  NAME
 *      StorePartitionSignatureToNVM
 *
 *  DESCRIPTION
 *      This function is used to store footer signatures in NVM
 *
 *  RETURNS:
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void StorePartitionSignatureToNVM(uint8* p_sig_data)
{
    MemCopy(&appOtaData.cur_footer[PARTITION_FOOTER_SIGNATURE_OFFSET],p_sig_data,
            PARTITION_FOOTER_SIGNATURE_LENGTH );
    
    MemCopy(appOtaData.prev_footer,appOtaData.cur_footer,sizeof(appOtaData.prev_footer));
    
    Nvm_Write((uint16*)&appOtaData.cur_footer[PARTITION_FOOTER_SIGNATURE_OFFSET],
              PARTITION_FOOTER_SIGNATURE_LENGTH,
              appOtaData.nvm_offset + NVM_CUR_FOOTER_SIGNATURE_OFFSET);
    
    Nvm_Write((uint16*)&appOtaData.prev_footer[PARTITION_FOOTER_SIGNATURE_OFFSET],
              PARTITION_FOOTER_SIGNATURE_LENGTH,
              appOtaData.nvm_offset + NVM_PREV_FOOTER_SIGNATURE_OFFSET);

    GaiaOtauSetCommitStatus(TRUE);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauSetCommitStatus
 *
 *  DESCRIPTION
 *      This function sets the commit status of GAIA OTAu procedure
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void GaiaOtauSetCommitStatus(bool status)
{
    appOtaData.commit_done = status;
    Nvm_Write((uint16*)&appOtaData.commit_done,
              sizeof(appOtaData.commit_done),
              appOtaData.nvm_offset + NVM_OTAU_COMMIT_DONE_OFFSET);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauSetRelayStore
 *
 *  DESCRIPTION
 *      This function updates the store type/store_id to be used for relay
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void GaiaOtauSetRelayStore(bool commit_successful)
{
    if(commit_successful)
    {              
        if(appOtaData.cur_store_type == USER_STORE)
        {
            updatePreviousStore();
            /* Store signature */
            startSHA256sum();
        }
    }
    else
    {
        if(!((appOtaData.cur_store_type == appOtaData.prev_store_type) && 
             (appOtaData.cur_store_id = appOtaData.prev_store_id)))
        {
            appOtaData.cur_partition_data_length = appOtaData.prev_partition_data_length;
            appOtaData.cur_store_type = appOtaData.prev_store_type;
            appOtaData.cur_store_id = appOtaData.prev_store_id;
            MemCopy(appOtaData.cur_header,appOtaData.prev_header,sizeof(appOtaData.cur_header));
            MemCopy(appOtaData.cur_footer,appOtaData.prev_footer,sizeof(appOtaData.cur_footer));
            
            Nvm_Write((uint16*)&appOtaData.cur_partition_data_length,
                  sizeof(appOtaData.cur_partition_data_length),
                  appOtaData.nvm_offset + NVM_OTAU_CUR_PARTITION_SIZE_OFFSET);
            Nvm_Write((uint16*)&appOtaData.cur_store_type,
                     sizeof(uint16),
                     appOtaData.nvm_offset + NVM_OTAU_CUR_STORE_TYPE_OFFSET);
            Nvm_Write((uint16*)&appOtaData.cur_store_id,
                     sizeof(uint16),
                     appOtaData.nvm_offset + NVM_OTAU_CUR_STORE_ID_OFFSET);
            
            /* Write to NVM */
            Nvm_Write((uint16*)&appOtaData.cur_header[PARTITION_HEADER_BODY_OFFSET],
                  MESH_HEADER_BODY_SIZE,
                  appOtaData.nvm_offset + NVM_CUR_HEADER_BODY_OFFSET);
            
            /* Write Partition information to NVM */
            Nvm_Write((uint16*)&appOtaData.cur_header[PARTITION_LENGTH_OFFSET],
                      PARTITION_INFO_SIZE,
                      appOtaData.nvm_offset + NVM_CUR_PARTITION_INFO_OFFSET);

            Nvm_Write((uint16*)&appOtaData.cur_footer[PARTITION_FOOTER_SIGNATURE_OFFSET],
                      PARTITION_FOOTER_SIGNATURE_LENGTH,
                      appOtaData.nvm_offset + NVM_CUR_FOOTER_SIGNATURE_OFFSET);
            GaiaOtauSetCommitStatus(TRUE);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      SendLOTAnnouncePacket
 *
 *  DESCRIPTION
 *      This function is used to send LOT Announce Message
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void SendLOTAnnouncePacket(void)
{  
    TimerDelete(appOtaData.announce_tid);
    appOtaData.announce_tid = TIMER_INVALID;

    if(appOtaData.calculate_hash)
    {
        updatePreviousStore();
        setCalculateHashStatus(FALSE);
        startSHA256sum();
    }

    if((appOtaData.cur_store_id != 0) && (!appOtaData.app_connected) && (appOtaData.commit_done) &&
       ((appOtaData.cur_store_type == APP_STORE && appOtaData.cs_id == appOtaData.app_id) || 
        (appOtaData.cur_store_type == USER_STORE)) && 
       (GaiaOtauClientGetState() != STATE_VM_UPGRADE_WAIT_POST_TRANSFER_DISCONNECT)
        && (GaiaOtauClientGetState() != STATE_VM_UPGRADE_WAIT_POST_TRANSFER_RECONNECTION)
        && (IsAppAssociated()) && (!scanning_ongoing))
    {
        lot_announce_data.companycode = GetUint16FromArray(&appOtaData.cur_header[PARTITION_HEADER_COMPANY_CODE_OFFSET]);
        lot_announce_data.platformtype = appOtaData.cur_header[PARTITION_HEADER_PLATFORMTYPE_OFFSET];
        lot_announce_data.typeencoding = appOtaData.cur_header[PARTITION_HEADER_TYPEENCODING_OFFSET];
        lot_announce_data.imagetype = appOtaData.cur_header[PARTITION_HEADER_IMAGETYPE_OFFSET];
        lot_announce_data.size = (appOtaData.cur_partition_data_length/1024);
        lot_announce_data.objectversion = ((((uint16)(appOtaData.cur_header[PARTITION_HEADER_APP_VERSION_OFFSET] & 0x3F)) << 10) |
                                                     (((uint16)(appOtaData.cur_header[PARTITION_HEADER_APP_VERSION_OFFSET+ 1] & 0x0F)) << 6) |
                                                     ((uint16)(appOtaData.cur_header[PARTITION_HEADER_APP_VERSION_OFFSET + 2] & 0x3F)));
        lot_announce_data.targetdestination = 0;
    
        if(appOtaData.announce_count < ANNOUNCE_COUNT)
        {
            DEBUG_STR("\r\nLOT Announce Sent");
            LotModelSendAnnounce(&lot_announce_data);
            appOtaData.announce_tid = TimerCreate(ANNOUNCE_COUNT_INTERVAL,TRUE, 
                                           handleAnnounceTimerExpiry);
            appOtaData.announce_count++;
        }
        else
        {
            appOtaData.announce_count = 0;
        }
        
    }  
    else
    {
        appOtaData.announce_count = 0;
    }
    
    if((!appOtaData.app_connected) && (appOtaData.announce_count == 0) && 
       (IsAppAssociated()))
    {
        appOtaData.announce_tid = TimerCreate(ANNOUNCE_INTERVAL,TRUE, 
                                           handleAnnounceTimerExpiry);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauClientConfigStoreMsg
 *
 *  DESCRIPTION
 *      This function handles the config store messages
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void GaiaOtauClientConfigStoreMsg(msg_t *msg)
{
    configstore_msg_t *cs_msg = (configstore_msg_t*)msg;

    switch(msg->header.id)
    {
        /*
         * This will be returned when the GAIA OTAu library is initialised
         * and the CS key is read.
         */
        case CONFIG_STORE_READ_KEY_CFM:
        {
            if(cs_msg->body.read_key_cfm.status == STATUS_SUCCESS)
            {
                if(cs_msg->body.read_key_cfm.id == CS_ID_APP_STORE_ID)
                {
                    appOtaData.cs_id = cs_msg->body.read_key_cfm.value[0];
                    appOtaData.app_id = StoreUpdate_GetAppId().id;
                }
            }
        }
        break;

        default:
        break;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauClientHandleStoreUpdateMsg
 *
 *  DESCRIPTION
 *      This function handles the config store messages
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void GaiaOtauClientHandleStoreUpdateMsg(device_handle_id device_id, 
                                               store_update_msg_t *msg)
{ 
    switch(msg->header.id)
    {
        case STORE_UPDATE_SET_STORE_ID_CFM:
        {
            if(appOtaData.cs_id != appOtaData.app_id)
            {
                appOtaData.cs_id = appOtaData.app_id;
                if(appOtaData.app_connected)
                {
                    TimerDelete(appOtaData.announce_tid);
                    appOtaData.announce_tid = TIMER_INVALID;
                }
                setCalculateHashStatus(TRUE);
            }
        }
        break;
        case STORE_UPDATE_HASH_STORE_CFM:
        {
            int i;
            int MSB_sum, LSB_sum;
            status_t status = msg->body.hash_store_cfm.status;

            if(appOtaData.store_hash_request_sent)
                appOtaData.store_hash_request_sent = FALSE;
            else
                break;
                
            if ( status == sys_status_success )
            {
                /* sum this hash with the previous hash
                 * it is a byte sum, so split in to 8octet sums and don't carry
                 */
                for ( i = 0; i < SHA256_SIZE_WORDS; i++ )
                {
                    MSB_sum =   ((appOtaData.oem_signature.sha256_sum[i]&0xff00) +
                                (appOtaData.oem_signature.sha256_buffer[i]&0xff00) );
                    MSB_sum &= 0xff00;

                    LSB_sum =   ((appOtaData.oem_signature.sha256_sum[i]) +
                                (appOtaData.oem_signature.sha256_buffer[i]) );
                    LSB_sum &= 0x00ff;

                    appOtaData.oem_signature.sha256_sum[i] = MSB_sum | LSB_sum;
                }
                for ( i = 0; i < SHA256_SIZE_WORDS; i++)
                {
                    appOtaData.footer_signature[i * 2] = appOtaData.oem_signature.sha256_sum[i] >> 8;
                    appOtaData.footer_signature[(i * 2) + 1] = appOtaData.oem_signature.sha256_sum[i] & 0x00FF;
                }
                StorePartitionSignatureToNVM(appOtaData.footer_signature);
            }
        }
        break;

        default:
        break;
    }
}

#endif

