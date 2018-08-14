/*! \file gaia_client.h
 *  \brief This file defines header file for GAIA client protocol
 *
 *  %%fullcopyright(2016)
 *  %%version
 *  %%appversion
 */

#ifndef __GAIA_CLIENT_H__
#define __GAIA_CLIENT_H__

/*! \addtogroup GAIA_Client
 * @{
 */

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "cm_types.h"

/*=============================================================================*
 *  Public Definitions
 *============================================================================*/

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/
/*----------------------------------------------------------------------------
 *  GAIAClientProcessCommand
 *----------------------------------------------------------------------------*/
/*! \brief Process a GAIA command
 *
 * This function processess GAIA command
 * \param[in] device_id Device handle of the received gaia command
 * \param[in] payload_len Payload length
 * \param[in] payload Pointer to payload data
 * \returns Nothing
 *
 */
extern void GAIAClientProcessCommand(device_handle_id device_id, 
                                     uint16 payload_len,
                                     uint8 *payload);

/*----------------------------------------------------------------------------
 *  GaiaClientSendCommandPacket
 *----------------------------------------------------------------------------*/
/*! \brief Prepares a GAIA packet with header and given payload
 *
 *  Prepares a GAIA packet with header and given payload and sends BLE
 *  notification to host
 * \param[in] device_id Device handle of the target device
 * \param[in] size_payload Payload length
 * \param[in] payload Pointer to payload data
 * \returns Nothing
 *
 */
extern void GaiaClientSendCommandPacket(device_handle_id device_id, uint16 size_payload, uint8 *payload);

/*----------------------------------------------------------------------------
 *  GaiaClientSendDisconnectPacket
 *----------------------------------------------------------------------------*/
/*! \brief Sends VM Disconnect
 *
 *  Disconnects the Link
 * \param[in] device_id Device handle of the target device to be disconnected
 * \returns Nothing
 *
 */
extern void GaiaClientSendDisconnectPacket(device_handle_id device_id);

/*----------------------------------------------------------------------------
 *  GaiaClientSendConnectPacket
 *----------------------------------------------------------------------------*/
/*! \brief Sends VM Connect
 *
 *  Connects the link
 * \param[in] device_id Device handle of the target device to be connected
 * \returns Nothing
 *
 */
extern void GaiaClientSendConnectPacket(device_handle_id device_id);

/*----------------------------------------------------------------------------
 *  GaiaClientSendAcknowledgement
 *----------------------------------------------------------------------------*/
/*! \brief Sends Acknowledgement
 *
 * \param[in] device_id Device handle of the target device
 * \param[in] status status of operation
 * \returns Nothing
 *
 */
extern void GaiaClientSendAcknowledgement(device_handle_id device_id,
                                          uint8 status);

#endif /* __GAIA_CLIENT_H__ */

