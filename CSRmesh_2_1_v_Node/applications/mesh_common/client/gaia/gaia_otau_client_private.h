/*! \file gaia_otau_client_private.h
 * \brief Private defines for the GAIA OTAu module
 *
 * %%fullcopyright(2016)
 * %%version
 * %%appversion
 *
 */
 
#ifndef GAIA_OTAU_CLIENT_PRIVATE_H_
#define GAIA_OTAU_CLIENT_PRIVATE_H_

/*=============================================================================*
 *  SDK Header Files
 *============================================================================*/
#include <types.h>
#include <debug.h>
#include "cm_types.h"

/*=============================================================================*
 *  Private Definitions
 *============================================================================*/

#define UPGRADE_HOSTACTION_YES                                  0
#define UPGRADE_HOST_TRANSFER_COMPLETE_RSP_BYTE_SIZE            1
#define UPGRADE_HOST_COMMIT_CFM_BYTE_SIZE                       1
#define VM_CONTROL_COMMAND_SIZE                                (3)
#define UPGRADE_MAX_DATA_BYTES_SENT                            (12) // (20 -8)
#define UPGRADE_HOST_START_REQ_SIZE                            (0)
#define UPGRADE_HOST_ABORT_REQ_SIZE                            (0)
#define UPGRADE_HOST_START_DATA_REQ_SIZE                       (0)
#define UPGRADE_HOST_IS_CSR_VALID_DONE_REQ_SIZE                (0)
#define UPGRADE_HOST_INPROGRESS_RES_BYTE_SIZE                  (1)
#define UPGRADE_HOST_CONTINUE_UPGRADE                          (0)
#define UPGRADE_HOST_SYNC_REQ_BYTE_SIZE                        (4)
#define GAIA_VMUPGRADE_POST_TRANSFER_REBOOT_DELAY              (2000000)

#ifdef GAIA_CLIENT_OTAU_DEBUG_ENABLE
#define DEBUG_STR(s)  DebugWriteString(s)
#define DEBUG_U32(u)  DebugWriteUint32(u)
#define DEBUG_U16(u)  DebugWriteUint16(u)
#define DEBUG_U8(u)   DebugWriteUint8(u)
#define DEBUG_TEST_U8_ARR(x,offset,n)  do { \
    uint16 debug_len = offset; \
    while( debug_len < n) \
    { \
        DebugWriteUint8(x[debug_len]); \
        debug_len++; \
    } \
}while(0)
#else
#define DEBUG_STR(s)
#define DEBUG_U32(u)
#define DEBUG_U16(u)
#define DEBUG_U8(u)
#define DEBUG_TEST_U8_ARR(x,offset,n)
#endif /* GAIA_CLIENT_OTAU_DEBUG_ENABLE */

/*=============================================================================*
 *  Private Data Types
 *============================================================================*/

/*----------------------------------------------------------------------------
 *  HandleUpgradeDisconnectAck
 *----------------------------------------------------------------------------*/
/*! \brief Handles the GAIA upgrade disconnect ack command
 *
 * \param[in] device_id Device handle of the target device
 * \param[in] payload_length payload length received 
 * \param[in] payload payload received through notifications
 * \returns Nothing
 *
 */
extern void HandleUpgradeDisconnectAck(device_handle_id device_id, 
                                       uint16 payload_length,
                                       uint8* payload);

/*----------------------------------------------------------------------------
 *  HandleUpgradeControlRequestAck
 *----------------------------------------------------------------------------*/
/*! \brief Handles the GAIA upgrade control request acknowledgement command
 *
 * \param[in] device_id Device handle of the target device
 * \param[in] payload_length payload length received 
 * \param[in] payload payload received through notifications
 * \returns Nothing
 *
 */
extern void HandleUpgradeControlRequestAck(device_handle_id device_id, 
                                           uint16 payload_length,
                                           uint8* payload);

/*----------------------------------------------------------------------------
 *  HandleUpgradeConnectAck
 *----------------------------------------------------------------------------*/
/*! \brief Handles the GAIA connect ack message
 *
 * \param[in] device_id Device handle of the target device
 * \param[in] payload_length payload length received 
 * \param[in] payload payload received through notifications
 * \returns Nothing
 *
 */
extern void HandleUpgradeConnectAck(device_handle_id device_id, 
                                    uint16 payload_length,
                                    uint8* payload);

/*----------------------------------------------------------------------------
 *  HandleGaiaResponseNotification
 *----------------------------------------------------------------------------*/
/*! \brief Handles the GAIA command Response messages received as notifications
 *
 * \param[in] device_id Device handle of the target device
 * \param[in] payload_len payload length received 
 * \param[in] payload payload received through notifications
 * \returns Nothing
 *
 */
extern void HandleGaiaResponseNotification(device_handle_id device_id,
                                           uint16 payload_len,
                                           uint8 *payload);

#endif /* GAIA_OTAU_CLIENT_PRIVATE_H_ */

