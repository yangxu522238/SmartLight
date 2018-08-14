/*! \file gaia_client_service.h
 *  \brief Header file for the GAIA client Service
 *
 *  %%fullcopyright(2016)
 *  %%version
 *  %%appversion
 *
 *
 */

#ifndef __GAIA_CLIENT_SERVICE_H__
#define __GAIA_CLIENT_SERVICE_H__

/*! \addtogroup GAIA_Client_Service
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
 *  Public Data Types
 *============================================================================*/

/* Configuration states */
typedef enum
{
    STATE_INIT = 0,
    STATE_GAIA_CONFIG_RESPONSE_CHAR,
    STATE_GAIA_CONFIGURED,
    STATE_GAIA_CONFIG_FAILED,
} GAIA_CLIENT_CONFIG_STATE_T;

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/
/*----------------------------------------------------------------------------
 *  GaiaInitClientService
 *----------------------------------------------------------------------------*/
/*! \brief Initialises the GAIA Service client
 *
 * This function initialises the ANCS Service client
 * \param[in] nvm_start_fresh Boolean variable to check if NVM has been not been written before
 * \returns Nothing
 *
 */
extern void GaiaInitClientService(bool nvm_start_fresh);

/*----------------------------------------------------------------------------
 *  GaiaClientIsServiceDiscovered
 *----------------------------------------------------------------------------*/
/*! \brief Checks if GAIA service handles are discovered
 *
 * This function checks if GAIA service handles are discovered
 * \param[in] device_id Device handle
 * \returns TRUE if success,otherwise FALSE
 *
 */
extern bool GaiaClientIsServiceDiscovered(device_handle_id device_id);

/*----------------------------------------------------------------------------
 *  GaiaWriteCommandEndpoint
 *----------------------------------------------------------------------------*/
/*! \brief Function writes the GAIA command endpoint 
 *
 * This function writes the GAIA command endpoint
 * \param[in] device_id Device handle
 * \param[in] buf_len Length of the buffer containing the data
 * \param[in] buf_value The buffer containing the data to be sent
 * \returns TRUE if success,otherwise FALSE
 *
 */
extern bool GaiaWriteCommandEndpoint(device_handle_id device_id, 
                                     uint16 buf_len, uint8* buf_value);

/*----------------------------------------------------------------------------
 *  GaiaReadResponseEndpoint
 *----------------------------------------------------------------------------*/
/*! \brief Read the GAIA Response Endpoint
 *
 * This function reads the GAIA response endpoint characteristic
 * \param[in] device_id Device handle
 * \returns Boolean
 *
 */
extern bool GaiaReadResponseEndpoint(device_handle_id device_id);

/*----------------------------------------------------------------------------
 *  GaiaConfigureService
 *----------------------------------------------------------------------------*/
/*! \brief Configures the Gaia service
 *
 * This function configures the Gaia service
 * \param[in] device_id Device handle
 * \returns Nothing
 *
 */
extern void GaiaConfigureService(device_handle_id device_id);

/*!@} */
#endif /* __GAIA_CLIENT_SERVICE_H__ */
