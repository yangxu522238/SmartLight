/*! \file   gaia_otau_client.c
 * \brief   GAIA OTAu Client protocol implementation
 *
 *  This library provides an Over The Air upgrade service using GAIA Client
 *
 * %%fullcopyright(2016)
 * %%version
 * %%appversion
 *
 */
 

/*=============================================================================*
 *  SDK Header Files
 *============================================================================*/
#include <buf_utils.h>
#include <mem.h>
#include <timer.h>
#include <random.h>

/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "gaia_client.h"
#include "gaia.h"
#include "byte_utils.h"
#include "gaia_otau_private.h"
#include "gaia_otau_client_private.h"
#include "gaia_otau_client_api.h"
#include "gaia_client_service.h"
#include "cm_types.h"
#include "cm_api.h"

/*=============================================================================*
 *  Private Definitions
 *============================================================================*/
#define GAIA_OTAU_CLIENT_SERVICE_VERSION (0x0001)

/*============================================================================*
 *  Private Data Types
 *============================================================================*/

/* This structure defines the local data store for this module */
typedef struct {
    GAIA_VMUPGRADE_STATE                m_state;
    uint32                              m_inprogress_id;
    timer_id                            ota_tid;
    device_handle_id                    device_id;
    bool                                m_request_reached_eof;
    uint32                              requested_num_bytes;
    gaia_otau_client_event_handler      callback;       /* callback to the application event handler */
    bool                                otau_transfer_in_progress;
} OTA_SERVICE_LOCAL_DATA_T;

/*=============================================================================*
 *  Private Data
 *============================================================================*/

/* This local data store for this module */
static OTA_SERVICE_LOCAL_DATA_T otaData;

/*=============================================================================*
 *  Private Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      notifyApplication
 *
 *  DESCRIPTION
 *      Notify the application of OTAuclient  events via the callback function.
 *
 *  PARAMETERS
 *      event [in]      Type of event to notify
 *      *data [in,out]  Pointer to any event-specific data that should be sent
 *
 *  RETURNS
 *      Return status of the callback
 *
 *---------------------------------------------------------------------------*/
static sys_status notifyApplication(gaia_otau_client_event event, 
                                    GAIA_OTAU_CLIENT_EVENT_T *data)
{
    if (otaData.callback != NULL)
    {
        return otaData.callback(event, data);
    }

    return sys_status_failed;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeDeviceResetTransferState
 *
 *  DESCRIPTION
 *      This function resets the transfer state of GAIA OTAu Client and notifies 
 *      the application
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
static void upgradeDeviceResetTransferState(void)
{
    GaiaOtauClientResetTransferState();
    notifyApplication(gaia_otau_client_reset_event, NULL);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeDeviceHandleStartCfm
 *
 *  DESCRIPTION
 *      This function handles the Start Confirm command
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeDeviceHandleStartCfm(device_handle_id device_id, 
                                        uint16 payload_len,
                                        uint8 *payload)
{
    if (payload_len == UPGRADE_HOST_START_CFM_PAYLOAD_SIZE)
    {
        uint8 status = payload[0];

        if (otaData.m_state == STATE_VM_UPGRADE_START_REQ)
        {
            // Acknowledge
            GaiaClientSendAcknowledgement(device_id,GAIA_STATUS_SUCCESS);
            DEBUG_STR("\r\nStart Confirm Command Received");

            if (status == UPGRADE_HOST_SUCCESS)
            {
                // Application ready for upgrade,send start data request
                uint16 byte_index=0;
                uint8 response[VM_CONTROL_COMMAND_SIZE + UPGRADE_HOST_START_DATA_REQ_SIZE];
                GAIA_OTAU_CLIENT_UPGRADE_START_EVENT_T start_evt;

                byte_index += SetUint8InArray(response,byte_index,UPGRADE_HOST_START_DATA_REQ);
                byte_index += SetUint16InArray(response,byte_index,UPGRADE_HOST_START_DATA_REQ_SIZE);
                GaiaClientSendCommandPacket(device_id,
                    VM_CONTROL_COMMAND_SIZE + UPGRADE_HOST_START_DATA_REQ_SIZE,
                    response);
                otaData.m_state = STATE_VM_UPGRADE_DATA_REQ;
                start_evt.device_id = device_id;
                notifyApplication(gaia_otau_client_upgrade_start_event,
                                 (GAIA_OTAU_CLIENT_EVENT_T*)&start_evt);
                DEBUG_STR("\r\nStart Data Request Command Sent");
            }
        }
        else if (otaData.m_state == STATE_VM_UPGRADE_START_REQ_AFTER_REBOOT)
        {
            // Acknowledge
            GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_SUCCESS);

            DEBUG_STR("\r\nStart Confirm Command Post Reboot Received");
            if (status == UPGRADE_HOST_SUCCESS)
            {
                // Application ready for commit then send InProgressRes message with ABORT = N
                uint16 byte_index=0;
                uint8 response[VM_CONTROL_COMMAND_SIZE + UPGRADE_HOST_INPROGRESS_RES_BYTE_SIZE];
                byte_index += SetUint8InArray(response,byte_index,UPGRADE_HOST_IN_PROGRESS_RES);
                byte_index += SetUint16InArray(response,byte_index,UPGRADE_HOST_INPROGRESS_RES_BYTE_SIZE);
                byte_index += SetUint8InArray(response,byte_index,UPGRADE_HOST_CONTINUE_UPGRADE);
                GaiaClientSendCommandPacket(device_id, VM_CONTROL_COMMAND_SIZE + UPGRADE_HOST_INPROGRESS_RES_BYTE_SIZE,
                                            response);
                otaData.m_state = STATE_VM_UPGRADE_IN_PROGRESS_REQ;
                DEBUG_STR("\r\nIn Progress Response Command Sent");
            }
        }
        else
        {
            GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INCORRECT_STATE);
        }
    }
    else
    {
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeSendData
 *
 *  DESCRIPTION
 *      This function sends the upgrade data
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeSendData(device_handle_id device_id, 
                            uint32 requested_num_bytes)
{
    GAIA_OTAU_CLIENT_DATA_REQ_EVENT_T transfer_data_evt;

    /* If the requested data is more than the max data bytes that could be sent
     * then the remaining data stored in requested_num_bytes is sent in the 
     * next packet.
     */
    if(requested_num_bytes > UPGRADE_MAX_DATA_BYTES_SENT)
    {
        requested_num_bytes = UPGRADE_MAX_DATA_BYTES_SENT;
        otaData.requested_num_bytes -= UPGRADE_MAX_DATA_BYTES_SENT;
    }
    else
    {
        otaData.requested_num_bytes = 0;
    }
    transfer_data_evt.no_bytes_requested = requested_num_bytes;
    transfer_data_evt.device_id = device_id;

    notifyApplication(gaia_otau_client_data_req_event,
                     (GAIA_OTAU_CLIENT_EVENT_T*)&transfer_data_evt);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeDeviceHandleDataBytesReq
 *
 *  DESCRIPTION
 *      This function handles the data bytes request
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeDeviceHandleDataBytesReq(device_handle_id device_id, 
                                            uint16 payload_len,
                                            uint8 *payload)
{
    DEBUG_STR("\r\nData Bytes Request Received");
    if (otaData.m_state == STATE_VM_UPGRADE_DATA_REQ)
    {
        if (payload_len == UPGRADE_HOST_DATA_BYTES_REQ_BYTE_SIZE)
        {
            otaData.requested_num_bytes = GetUint32FromArray(&payload[0]);
            otaData.m_request_reached_eof = FALSE;
            upgradeSendData(device_id, otaData.requested_num_bytes);
        }
        else
        {
           GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INCORRECT_STATE);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeDeviceHandleAbortCfm
 *
 *  DESCRIPTION
 *      This function handles the abort confirm
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeDeviceHandleAbortCfm(device_handle_id device_id, 
                                        uint16 payload_len, uint8 *payload)
{
    GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_SUCCESS);
    otaData.m_state = STATE_VM_UPGRADE_ABORTING_DISCONNECT;
    GaiaClientSendDisconnectPacket(device_id);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeDeviceHandleTransferCompleteInd
 *
 *  DESCRIPTION
 *      This function handles the transfer complete indication
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeDeviceHandleTransferCompleteInd(device_handle_id device_id,
                                                   uint16 payload_len,
                                                   uint8 *payload)
{
    if (otaData.m_state == STATE_VM_UPGRADE_TRANSFER ||
        otaData.m_state == STATE_VM_UPGRADE_WAIT_VALIDATION_RSP || 
        otaData.m_state == STATE_VM_UPGRADE_WAIT_VALIDATION)
    {
        if(otaData.m_state == STATE_VM_UPGRADE_WAIT_VALIDATION && 
           otaData.ota_tid != TIMER_INVALID)
        {
            TimerDelete(otaData.ota_tid);
            otaData.ota_tid = TIMER_INVALID;
        }
        DEBUG_STR("\r\nTransfer Complete Indication Received ");
        // device indicated that it has successfully received and validated all image data
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_SUCCESS);

        // Send Transfer Complete Res message, continue with upgrade = TRUE
        uint16 byte_index=0;
        uint8 response[VM_CONTROL_COMMAND_SIZE + UPGRADE_HOST_TRANSFER_COMPLETE_RSP_BYTE_SIZE];
        byte_index += SetUint8InArray(response,byte_index,UPGRADE_HOST_TRANSFER_COMPLETE_RES);
        byte_index += SetUint16InArray(response,byte_index,UPGRADE_HOST_TRANSFER_COMPLETE_RSP_BYTE_SIZE);
        byte_index += SetUint8InArray(response,byte_index,UPGRADE_HOSTACTION_YES);
        GaiaClientSendCommandPacket(device_id,
                                    VM_CONTROL_COMMAND_SIZE + 
                                    UPGRADE_HOST_TRANSFER_COMPLETE_RSP_BYTE_SIZE,
                                    response);
        otaData.m_state = STATE_VM_UPGRADE_WAIT_POST_TRANSFER_DISCONNECT;
        DEBUG_STR("\r\nTransfer Complete Response Sent ");
    }
    else
    {
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INCORRECT_STATE);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeDeviceHandleCommitReq
 *
 *  DESCRIPTION
 *      This function sends the commit confirm
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeDeviceHandleCommitReq(device_handle_id device_id, 
                                         uint16 payload_len, uint8 *payload)
{
    if (otaData.m_state == STATE_VM_UPGRADE_IN_PROGRESS_REQ)
    {
        // Acknowledge
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_SUCCESS);

        DEBUG_STR("\r\nCommit Request Received ");
        // Send Transfer Complete Res message, continue with upgrade = TRUE
        uint16 byte_index=0;
        uint8 response[VM_CONTROL_COMMAND_SIZE + UPGRADE_HOST_COMMIT_CFM_BYTE_SIZE];

        byte_index += SetUint8InArray(response,byte_index,UPGRADE_HOST_COMMIT_CFM);
        byte_index += SetUint16InArray(response,byte_index,UPGRADE_HOST_COMMIT_CFM_BYTE_SIZE);
        byte_index += SetUint8InArray(response,byte_index,UPGRADE_HOST_CONTINUE_UPGRADE);
        GaiaClientSendCommandPacket(device_id, VM_CONTROL_COMMAND_SIZE + UPGRADE_HOST_COMMIT_CFM_BYTE_SIZE,
                                    response);
        otaData.m_state = STATE_VM_UPGRADE_COMMIT_REQ;
        DEBUG_STR("\r\nCommit Confirm Sent ");
    }
    else
    {
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INCORRECT_STATE);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeHandleErrorWarnInd
 *
 *  DESCRIPTION
 *      This function handles the error and warning indication
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeHandleErrorWarnInd(device_handle_id device_id, 
                                      uint16 payload_len, uint8 *payload)
{
    if (payload_len == UPGRADE_HOST_ERRORWARN_IND_BYTE_SIZE)
    {
        DEBUG_STR("\r\nError Indication Received ");
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_SUCCESS);

        uint16 error_code = GetUint16FromArray(payload);
        // Send Transfer Complete Res message, continue with upgrade = TRUE
        if (error_code != UPGRADE_HOST_SUCCESS)
        {
            GAIA_OTAU_CLIENT_UPGRADE_END_EVENT_T upgrade_end_evt;
            upgrade_end_evt.device_id = device_id;

            //Error code is not success, abort and reset
            upgradeDeviceResetTransferState();

            notifyApplication(gaia_otau_client_upgrade_end_event,
                             (GAIA_OTAU_CLIENT_EVENT_T*)&upgrade_end_evt);
        }
    }
    else
    {
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeDeviceHandleCompleteInd
 *
 *  DESCRIPTION
 *      This function indicates that update has been completed
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeDeviceHandleCompleteInd(device_handle_id device_id, 
                                           uint16 payload_len, uint8 *payload)
{
    if (otaData.m_state == STATE_VM_UPGRADE_COMMIT_REQ)
    {
        DEBUG_STR("\r\nTransfer Completed");
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_SUCCESS);
        otaData.otau_transfer_in_progress = FALSE;

        // Upgrade completed,send GAIA Disconnect
        GaiaClientSendDisconnectPacket(device_id);
        otaData.m_state = STATE_VM_UPGRADE_DISCONNECT;
    }
    else
    {
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INCORRECT_STATE);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeDeviceSendStartReq
 *
 *  DESCRIPTION
 *      This function handles the sync confirm command
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeDeviceSendStartReq(device_handle_id device_id)
{
    uint16 byte_index = 0;
    uint8 response[VM_CONTROL_COMMAND_SIZE];
    byte_index += SetUint8InArray(response,byte_index,UPGRADE_HOST_START_REQ);
    byte_index += SetUint16InArray(response,byte_index,UPGRADE_HOST_START_REQ_SIZE);
    GaiaClientSendCommandPacket(device_id, VM_CONTROL_COMMAND_SIZE,response);
    DEBUG_STR("\r\nStart Request Sent ");
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeDeviceHandleSyncCfm
 *
 *  DESCRIPTION
 *      This function handles the sync confirm command
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeDeviceHandleSyncCfm(device_handle_id device_id, 
                                       uint16 payload_len, uint8 *payload)
{
    if (otaData.m_state == STATE_VM_UPGRADE_SYNC_REQ)
    {
        if (payload_len == UPGRADE_HOST_SYNC_CFM_BYTE_SIZE)
        {
            uint8 resumePoint = payload[0];
            uint32 inProgressId = GetUint32FromArray(&payload[1]);
            uint8 procotolVersion = payload[5];
            
            if(procotolVersion == PROTOCOL_VERSION_3)
            {
                if(inProgressId == otaData.m_inprogress_id)
                {
                    DEBUG_STR("\r\nSync Confirm Received ");
                    GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_SUCCESS);
                    notifyApplication(gaia_otau_client_reset_event, NULL);

                    if(resumePoint == UPGRADE_RESUME_POINT_POST_REBOOT)
                    {
                        otaData.m_state = STATE_VM_UPGRADE_START_REQ_AFTER_REBOOT;
                    }
                    else if(resumePoint == UPGRADE_RESUME_POINT_START)
                    {
                        otaData.m_state = STATE_VM_UPGRADE_START_REQ;
                    }
                    upgradeDeviceSendStartReq(device_id);
                }
                else
                {
                    GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INVALID_PARAMETER);

                    /* As the sync id has not matched abort this transfer */
                    uint16 byte_index = 0;
                    uint8 response[VM_CONTROL_COMMAND_SIZE];
                    byte_index += SetUint8InArray(response,byte_index,UPGRADE_HOST_ABORT_REQ);
                    byte_index += SetUint16InArray(response,byte_index,UPGRADE_HOST_ABORT_REQ_SIZE);
                    GaiaClientSendCommandPacket(device_id, VM_CONTROL_COMMAND_SIZE,response);
                    DEBUG_STR("\r\nAbort Request Sent as sync id mismatch ");
                }
            }
            else
            {
                GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INVALID_PARAMETER);
            }
        }
        else
        {
            GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INCORRECT_STATE);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeSendIsValidationDoneRequest
 *
 *  DESCRIPTION
 *      This function sends the validation done request
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeSendIsValidationDoneRequest(device_handle_id device_id)
{
    DEBUG_STR("\nSend Validation Request");
    uint16 byte_index=0;
    uint8 response[VM_CONTROL_COMMAND_SIZE];

    byte_index += SetUint8InArray(response,byte_index,UPGRADE_HOST_IS_CSR_VALID_DONE_REQ);
    byte_index += SetUint16InArray(response,byte_index,UPGRADE_HOST_IS_CSR_VALID_DONE_REQ_SIZE);
    GaiaClientSendCommandPacket(device_id, 
                                VM_CONTROL_COMMAND_SIZE,
                                response);
    otaData.m_state = STATE_VM_UPGRADE_WAIT_VALIDATION_RSP;
}

/*-----------------------------------------------------------------------------*
 *  NAME
 *      handleValidationTimerExpiry
 *
 *  DESCRIPTION
 *      This function handles the expiry of validation timer
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void handleValidationTimerExpiry(timer_id tid)
{
    if(otaData.ota_tid == tid)
    {
        if(otaData.m_state == STATE_VM_UPGRADE_WAIT_VALIDATION)
        {
            if(otaData.device_id != CM_INVALID_DEVICE_ID)
            {
                DEBUG_STR("\r\nIs Validation Done Request Sent");
                upgradeSendIsValidationDoneRequest(otaData.device_id);
            }
        }
        otaData.ota_tid = TIMER_INVALID;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      upgradeDeviceHandleIsValidationDoneCfm
 *
 *  DESCRIPTION
 *      This function handles the validation done confirmation
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
static void upgradeDeviceHandleIsValidationDoneCfm(device_handle_id device_id, 
                                                   uint16 payload_len,
                                                   uint8 *payload)
{
    if (otaData.m_state == STATE_VM_UPGRADE_WAIT_VALIDATION_RSP)
    {
        if (payload_len == UPGRADE_HOST_IS_CSR_VALID_DONE_CFM_BYTE_SIZE)
        {
            DEBUG_STR("\r\nValidation Done Confirm Received");
            uint8 back_off_time_ms = GetUint16FromArray(payload);
            GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_SUCCESS);
            otaData.m_state = STATE_VM_UPGRADE_WAIT_VALIDATION;
            otaData.device_id = device_id;
            otaData.ota_tid = TimerCreate(back_off_time_ms * 1000,TRUE, 
                                          handleValidationTimerExpiry);
        }
        else
        {
            GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INVALID_PARAMETER);
        }
    }
    else
    {
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INCORRECT_STATE);
    }
}

/*=============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauClientInitEvent
 *
 *  DESCRIPTION
 *      This function is used to initialise upgrade client data
 *      structure.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void GaiaOtauClientInitEvent(void)
{
    /* Initialise otaData */
    upgradeDeviceResetTransferState();
    otaData.otau_transfer_in_progress = FALSE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauClientConnNotifyEvent
 *
 *  DESCRIPTION
 *      This function is used to handle a connection event
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void GaiaOtauClientConnNotifyEvent(CM_CONNECTION_NOTIFY_T *cm_event_data)
{
    GAIA_OTAU_CLIENT_CONN_EVENT_T conn_event;

    conn_event.device_id = cm_event_data->device_id;
    conn_event.reason = cm_event_data->reason;
    conn_event.result = cm_event_data->result;
    notifyApplication(gaia_otau_client_conn_event,
                      (GAIA_OTAU_CLIENT_EVENT_T*)&conn_event);

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauClientRegisterCallback
 *
 *  DESCRIPTION
 *      Set the callback so the OTAu Client library can inform the application
 *      of the progress and warn about special events.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void GaiaOtauClientRegisterCallback(gaia_otau_client_event_handler callback)
{
    otaData.callback = callback;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauClientStartUpgrade
 *
 *  DESCRIPTION
 *      This function starts the Gaia Otau Client upgrade process 
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void GaiaOtauClientStartUpgrade(device_handle_id device_id)
{
    otaData.m_state = STATE_VM_UPGRADE_CONNECT;
    DEBUG_STR("\r\nUpgrade Connect Request Sent");
    GaiaClientSendConnectPacket(device_id);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauClientSendData
 *
 *  DESCRIPTION
 *      This function sends the GAIA Otau data passed
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
extern void GaiaOtauClientSendData(device_handle_id device_id, uint8* data,
                                   uint32 data_length, bool reached_eof)
{
    uint16 byte_index=0;
    uint8 response[VM_CONTROL_COMMAND_SIZE + UPGRADE_MAX_DATA_BYTES_SENT + 1];
    
    if (otaData.m_state == STATE_VM_UPGRADE_DATA_REQ || 
        otaData.m_state == STATE_VM_UPGRADE_TRANSFER)
    {
        otaData.m_request_reached_eof = reached_eof;
        byte_index += SetUint8InArray(response, byte_index, UPGRADE_HOST_DATA);
        byte_index += SetUint16InArray(response, byte_index, data_length);
        MemCopy(&response[byte_index], data, data_length);
        GaiaClientSendCommandPacket(device_id, 
                                 VM_CONTROL_COMMAND_SIZE + data_length,
                                 response);
        otaData.m_state = STATE_VM_UPGRADE_TRANSFER;
        DEBUG_STR("\r\nData Bytes Sent");
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauClientGetState
 *
 *  DESCRIPTION
 *      This function returns the transfer state of GAIA OTAu
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern uint16 GaiaOtauClientGetState(void)
{
    return otaData.m_state;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauClientResetTransferState
 *
 *  DESCRIPTION
 *      This function resets the transfer state of GAIA OTAu Client
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void GaiaOtauClientResetTransferState(void)
{
    otaData.m_request_reached_eof = FALSE;
    otaData.requested_num_bytes = 0;
    TimerDelete(otaData.ota_tid);
    otaData.ota_tid = TIMER_INVALID;
    otaData.m_state = STATE_VM_UPGRADE_IDLE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauClientUpdateConnStatus
 *
 *  DESCRIPTION
 *      This function is used to handle a connection event on the GAIA OTAU 
 *      Client.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
extern void GaiaOtauClientUpdateConnStatus(device_handle_id device_id, 
                                           cm_conn_result result)
{
    if(result != cm_conn_res_success)
    {
        if(otaData.device_id == device_id)
        {
            otaData.device_id = CM_INVALID_DEVICE_ID;
        }
    }

    if(CMGetPeerDeviceRole(device_id) == con_role_peripheral)
    {
        if(result == cm_conn_res_success)
        {
            if(otaData.m_state == STATE_VM_UPGRADE_WAIT_POST_TRANSFER_RECONNECTION)
            {
                DEBUG_STR("\r\nPost Transfer Reconnection Done");
                otaData.m_state = STATE_VM_UPGRADE_IDLE;
            }
            else
            {
                DEBUG_STR("\r\nConnection Done");
            }
        }
        else if(result == cm_disconn_res_success)
        {
            DEBUG_STR("\r\nDisconnection Done");
            switch (otaData.m_state)
            {
                case STATE_VM_UPGRADE_WAIT_POST_TRANSFER_DISCONNECT:
                case STATE_VM_UPGRADE_WAIT_POST_TRANSFER_WAIT_DISCONNECT:
                    DEBUG_STR("\r\nPost Transfer Connect Request Sent");
                    otaData.m_state = STATE_VM_UPGRADE_WAIT_POST_TRANSFER_RECONNECTION;
                break;

                case STATE_VM_UPGRADE_DISCONNECT:
                case STATE_VM_UPGRADE_COMPLETED:
                    upgradeDeviceResetTransferState();
                break;

                case STATE_VM_UPGRADE_IDLE:
                    upgradeDeviceResetTransferState();
                break;

                default:
                    upgradeDeviceResetTransferState();
                break;
             }
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GaiaOtauClientConfigurationComplete
 *
 *  DESCRIPTION
 *      The function is called by the Gaia Client Service once the configuration
 *      of the device is complete.
 *
 *  RETURNS/MODIFIES
 *      Nothing
 *
 *----------------------------------------------------------------------------*/
extern void GaiaOtauClientConfigurationComplete(device_handle_id device_id,
                                                uint16 config_state)
{
    GAIA_OTAU_CLIENT_CONFIG_STATUS_EVENT_T config_evt;
    config_evt.device_id = device_id;
    config_evt.status = config_state;
    notifyApplication(gaia_otau_client_config_status_event,
                      (GAIA_OTAU_CLIENT_EVENT_T*)&config_evt);
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      HandleUpgradeConnectAck
 *
 *  DESCRIPTION
 *      The device handles upgrade conenct acknowledgement
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
extern void HandleUpgradeConnectAck(device_handle_id device_id, 
                                    uint16 payload_length, uint8* payload)
{
    if (otaData.m_state == STATE_VM_UPGRADE_CONNECT)
    {
        uint8 status = payload[0];
        if(status == GAIA_STATUS_SUCCESS)
        {
            DEBUG_STR("\r\nUpgrade Connect Request Received");
            // Send Sync Request
            uint16 byte_index=0;
            uint8 response[VM_CONTROL_COMMAND_SIZE + UPGRADE_HOST_SYNC_REQ_BYTE_SIZE];
            if(otaData.otau_transfer_in_progress == FALSE)
            {
                otaData.m_inprogress_id = Random32();
                otaData.otau_transfer_in_progress = TRUE;
            }

            byte_index += SetUint8InArray(response,byte_index,UPGRADE_HOST_SYNC_REQ);
            byte_index += SetUint16InArray(response,byte_index,UPGRADE_HOST_SYNC_REQ_BYTE_SIZE);
            byte_index += SetUint32InArray(response,byte_index,otaData.m_inprogress_id);
            GaiaClientSendCommandPacket(device_id, 
                                        VM_CONTROL_COMMAND_SIZE + UPGRADE_HOST_SYNC_REQ_BYTE_SIZE,
                                        response);

            otaData.m_state = STATE_VM_UPGRADE_SYNC_REQ;
            DEBUG_STR("\r\nSync Request Sent");
        }
        else
        {
            otaData.m_state = STATE_VM_UPGRADE_IDLE;
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HandleUpgradeControlRequestAck
 *
 *  DESCRIPTION
 *      This function handles upgrade control request acknowledgement
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
extern void HandleUpgradeControlRequestAck(device_handle_id device_id, 
                                           uint16 payload_length,
                                           uint8* payload)
{
    switch (otaData.m_state)
    {
        case STATE_VM_UPGRADE_TRANSFER:
        {
            uint8 status = payload[0];
            if(status == GAIA_STATUS_SUCCESS)
            {
                if(otaData.m_request_reached_eof)
                {
                    otaData.m_state = STATE_VM_UPGRADE_WAIT_VALIDATION_RSP;
                    DEBUG_STR("\r\nValidation Request Sent");
                    upgradeSendIsValidationDoneRequest(device_id);
                }
                else
                {
                    if(otaData.requested_num_bytes > 0)
                    {
                        upgradeSendData(device_id, otaData.requested_num_bytes);
                    }
                    otaData.m_state = STATE_VM_UPGRADE_DATA_REQ;
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
 *      HandleUpgradeDisconnectAck
 *
 *  DESCRIPTION
 *      This function handles upgrade disconnect acknowledgement
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
extern void HandleUpgradeDisconnectAck(device_handle_id device_id,
                                       uint16 payload_length,
                                       uint8* payload)
{
    GAIA_OTAU_CLIENT_UPGRADE_END_EVENT_T upgrade_end_evt;
    upgrade_end_evt.device_id = device_id;

    upgradeDeviceResetTransferState();

    notifyApplication(gaia_otau_client_upgrade_end_event,
                     (GAIA_OTAU_CLIENT_EVENT_T*)&upgrade_end_evt);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HandleGaiaResponseNotification
 *
 *  DESCRIPTION
 *      This function handles the GAIA VM Upgrade Commands
 *
 *  RETURNS
 *      None
 *---------------------------------------------------------------------------*/
extern void HandleGaiaResponseNotification(device_handle_id device_id, 
                                           uint16 payload_len,
                                           uint8 *payload)
{
    if(payload_len >= 3)
    {
        /* Retrieve the msg id from the message */
        uint8 msg_id = GetUint8FromArray(payload);
        payload++;
        payload_len--;

        /* Retrieve the response length from the message */
        uint16 rsp_len = GetUint16FromArray(payload);
        payload += 2;
        payload_len -= 2;

        /* If payload length does not match response length, then we have not
         * received the expected packet hence send an ack with invalid param
         */
        if (payload_len != rsp_len)
        {
            GaiaClientSendAcknowledgement(device_id,
                                          GAIA_STATUS_INVALID_PARAMETER);
        }
        else
        {
            switch (msg_id)
            {
                case UPGRADE_HOST_SYNC_CFM:
                upgradeDeviceHandleSyncCfm(device_id, payload_len, payload);
                break;

                case UPGRADE_HOST_START_CFM:
                upgradeDeviceHandleStartCfm(device_id, payload_len, payload);
                break;

                case UPGRADE_HOST_DATA_BYTES_REQ:
                upgradeDeviceHandleDataBytesReq(device_id, payload_len, payload);
                break;

                case UPGRADE_HOST_ABORT_CFM:
                upgradeDeviceHandleAbortCfm(device_id, payload_len, payload);
                break;

                case UPGRADE_HOST_TRANSFER_COMPLETE_IND:
                upgradeDeviceHandleTransferCompleteInd(device_id, 
                                                       payload_len,
                                                       payload);
                break;

                case UPGRADE_HOST_COMMIT_REQ:
                upgradeDeviceHandleCommitReq(device_id, payload_len, payload);
                break;

                case UPGRADE_HOST_ERRORWARN_IND:
                upgradeHandleErrorWarnInd(device_id, payload_len, payload);
                break;

                case UPGRADE_HOST_COMPLETE_IND:
                upgradeDeviceHandleCompleteInd(device_id, payload_len, payload);
                break;

                case UPGRADE_HOST_IS_CSR_VALID_DONE_CFM:
                upgradeDeviceHandleIsValidationDoneCfm(device_id,
                                                       payload_len,
                                                       payload);
                break;

                default:
                GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_NOT_SUPPORTED);
                break;
            }
        }
    }
    else
    {
        GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_INVALID_PARAMETER);
    }
}
