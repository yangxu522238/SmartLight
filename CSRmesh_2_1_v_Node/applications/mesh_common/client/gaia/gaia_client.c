/******************************************************************************
 *  %%fullcopyright(2016)
 *  %%version
 *  %%appversion
 *
 * FILE
 *    gaia_client.c
 *
 *  DESCRIPTION
 *    This file implements the GAIA protocol for a client connection
 *
 ******************************************************************************/

/*=============================================================================*
 *  SDK Header Files
 *============================================================================*/
#include <mem.h>

/*=============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "gaia_otau_client_private.h"
#include "gaia_client_service.h"
#include "gaia_client.h"
#include "gaia.h"
#include "byte_utils.h"

/*============================================================================*
 *  Private Definitions
 *============================================================================*/
#define GAIA_CLIENT_SEND_ACK_PKT_SIZE                       (5)
#define GAIA_CLIENT_CONNECT_PKT_SIZE                        (4)
#define GAIA_CLIENT_DISCONNECT_PKT_SIZE                     (4)
#define GAIA_CLIENT_COMMAND_PKT_SIZE                        (20)
#define GAIA_CLIENT_COMMAND_HEADER_SIZE                     (4)

/*=============================================================================*
 *  Private Function Implementations
 *============================================================================*/

/*-----------------------------------------------------------------------------
 * NAME
 *     gaiaHandleNotificationCommand
 *
 * DESCRIPTION
 *     This function handles notification command
 *
 * RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/

static void gaiaHandleNotificationCommand(device_handle_id device_id,
                                          uint16 command_id,
                                          uint16 payload_length,
                                          uint8 *payload)
{
    uint8 event_code;
    event_code = *payload;
    payload++;
    payload_length--;

    switch (command_id)
    {
        case GAIA_EVENT_NOTIFICATION:
        {
            if(event_code == GAIA_EVENT_VMUP_PACKET)
            {
                HandleGaiaResponseNotification(device_id, payload_length, payload);
            }
            else
            {
                GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_NOT_SUPPORTED);
            }
        }
        break;

        default:
            GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_NOT_SUPPORTED);
        break;
    }
}

/*-----------------------------------------------------------------------------
 * NAME
 *      gaiaHandleDataTransferCommandAck
 *
 * DESCRIPTION
 *      Handle a Data Transfer command Acknowledgement
 *
 * RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/

static void gaiaHandleDataTransferCommandAck(device_handle_id device_id,
                                             uint16 command_id,
                                             uint16 payload_length,
                                             uint8 *payload)
{
     switch (command_id)
     {
        case (GAIA_COMMAND_VM_UPGRADE_CONNECT):
            HandleUpgradeConnectAck(device_id, payload_length, payload);
        break;

        case (GAIA_COMMAND_VM_UPGRADE_DISCONNECT):
            HandleUpgradeDisconnectAck(device_id, payload_length, payload);
        break;

        case (GAIA_COMMAND_VM_UPGRADE_CONTROL):
            HandleUpgradeControlRequestAck(device_id, payload_length, payload);
        break;

        default:
        break;
    }
}
/*=============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      GAIAClientProcessCommand
 *
 *  DESCRIPTION
 *      Process a GAIA command
 *
 *  RETURNS/MODIFIES
 *      None
 *----------------------------------------------------------------------------*/
extern void GAIAClientProcessCommand(device_handle_id device_id, 
                                     uint16 payload_len, uint8 *payload)
{
    uint16 command_id = 0;

    if (payload_len >= GAIA_GATT_VID_SIZE)
    {
        /* First two bytes contain vendor id in big-endian format */
        if (BufReadUint16BE(&payload) != GAIA_VENDOR_CSR)
        {
            return;
        }
        payload_len -= GAIA_GATT_VID_SIZE;
    }

    if (payload_len >= GAIA_GATT_COMMAND_ID_SIZE)
    {
        /* next two bytes contain commandId in big-endian format */
        command_id = BufReadUint16BE(&payload);
        payload_len -= GAIA_GATT_COMMAND_ID_SIZE;
    }

    if((HIGH(command_id) & GAIA_ACK_MASK_H))
    {
        switch (command_id & GAIA_COMMAND_TYPE_MASK)
        {
            case GAIA_COMMAND_TYPE_DATA_TRANSFER:
                command_id = command_id & (~GAIA_ACK_MASK);
                gaiaHandleDataTransferCommandAck(device_id, command_id,
                                                 payload_len, payload);
            break;

            default:
            break;
        }
    }
    else
    {
        switch (command_id & GAIA_COMMAND_TYPE_MASK)
        {
            case GAIA_COMMAND_TYPE_NOTIFICATION:
                gaiaHandleNotificationCommand(device_id, command_id,
                                              payload_len, payload);
            break;

            default:
                GaiaClientSendAcknowledgement(device_id, GAIA_STATUS_NOT_SUPPORTED);
            break;
        }
    }
}

/*-----------------------------------------------------------------------------
 *  NAME
 *      GaiaClientSendCommandPacket
 *
 *  DESCRIPTION
 *      Prepares a upgrade control GAIA packet with header and given payload and 
 *      sends BLE notification to host.
 *
 *  RETURNS:
 *      None
 *
 *----------------------------------------------------------------------------*/
extern void GaiaClientSendCommandPacket(device_handle_id device_id, 
                                            uint16 size_payload, uint8 *payload)
{
    uint8 data[GAIA_CLIENT_COMMAND_PKT_SIZE];
    uint8 *p = data;

    if(size_payload >(GAIA_CLIENT_COMMAND_PKT_SIZE - GAIA_CLIENT_COMMAND_HEADER_SIZE))
    {
        return;
    }

    /* Copy the header */
    *p++ = HIGH(GAIA_VENDOR_CSR);
    *p++ = LOW(GAIA_VENDOR_CSR);
    *p++ = HIGH(GAIA_COMMAND_VM_UPGRADE_CONTROL);
    *p++ = LOW(GAIA_COMMAND_VM_UPGRADE_CONTROL);

    /*  Copy in the payload */
    MemCopy(p, payload, size_payload);
    GaiaWriteCommandEndpoint(device_id, 
                             size_payload + GAIA_CLIENT_COMMAND_HEADER_SIZE,
                             data);
}

/*-----------------------------------------------------------------------------
 *  NAME
 *      GaiaClientSendDisconnectPacket
 *
 *  DESCRIPTION
 *      Prepares a GAIA upgrade disconnect packet with header and given payload 
 *      and sends BLE notification to host.
 *
 *  RETURNS:
 *      None
 *
 *----------------------------------------------------------------------------*/
extern void GaiaClientSendDisconnectPacket(device_handle_id device_id)
{
    uint8 data[GAIA_CLIENT_DISCONNECT_PKT_SIZE];
    uint8 *p = data;

    /* Fill GAIA Packet Header */
    *p++ = HIGH(GAIA_VENDOR_CSR);
    *p++ = LOW(GAIA_VENDOR_CSR);
    *p++ = HIGH(GAIA_COMMAND_VM_UPGRADE_DISCONNECT);
    *p++ = LOW(GAIA_COMMAND_VM_UPGRADE_DISCONNECT);

    GaiaWriteCommandEndpoint(device_id, GAIA_CLIENT_DISCONNECT_PKT_SIZE, data);
}

/*-----------------------------------------------------------------------------
 *  NAME
 *      GaiaClientSendConnectPacket
 *
 *  DESCRIPTION
 *      Prepares a GAIA upgrade connect packet with header and given payload and 
 *      sends BLE notification to host.
 *
 *  RETURNS:
 *      None
 *
 *----------------------------------------------------------------------------*/
extern void GaiaClientSendConnectPacket(device_handle_id device_id)
{
    uint8 data[GAIA_CLIENT_CONNECT_PKT_SIZE];
    uint8 *p = data;

    /* Fill GAIA Packet Header */
    *p++ = HIGH(GAIA_VENDOR_CSR);
    *p++ = LOW(GAIA_VENDOR_CSR);
    *p++ = HIGH(GAIA_COMMAND_VM_UPGRADE_CONNECT);
    *p++ = LOW(GAIA_COMMAND_VM_UPGRADE_CONNECT);

    GaiaWriteCommandEndpoint(device_id, GAIA_CLIENT_CONNECT_PKT_SIZE, data);
}

/*-----------------------------------------------------------------------------
 *  NAME
 *      GaiaClientSendAcknowledgement
 *
 *  DESCRIPTION
 *      Sends acknowledgement of received GAIA upgrade control packets
 *
 *  RETURNS:
 *      None
 *
 *----------------------------------------------------------------------------*/
extern void GaiaClientSendAcknowledgement(device_handle_id device_id, 
                                          uint8 status)
{
    uint8 data[GAIA_CLIENT_SEND_ACK_PKT_SIZE];
    uint8 *p = data;

    *p++ = HIGH(GAIA_VENDOR_CSR);
    *p++ = LOW(GAIA_VENDOR_CSR);
    *p++ = HIGH(GAIA_ACK_MASK | GAIA_COMMAND_VM_UPGRADE_CONTROL);
    *p++ = LOW(GAIA_ACK_MASK | GAIA_COMMAND_VM_UPGRADE_CONTROL);
    *p++ = status;

    GaiaWriteCommandEndpoint(device_id, GAIA_CLIENT_SEND_ACK_PKT_SIZE, data);

    if(status != GAIA_STATUS_SUCCESS)
    {
        GaiaClientSendDisconnectPacket(device_id);
    }
}

