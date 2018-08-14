/******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 * FILE
 *      user_config.h
 *
 * DESCRIPTION
 *      This file contains definitions which will enable customization of the
 *      application.
 *
 ******************************************************************************/

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

/*=============================================================================*
 *  Public Definitions
 *============================================================================*/
/* Application version */
#define APP_MAJOR_VERSION       (2)
#define APP_MINOR_VERSION       (1)
#define APP_NEW_VERSION         (0)

/* Application NVM version. This version is used to keep the compatibility of
 * NVM contents with the application version. This value needs to be modified
 * only if the new version of the application has a different NVM structure
 * than the previous version (such as number of groups supported) that can
 * shift the offsets of the currently stored parameters.
 * If the application NVM version has changed, it could still read the values
 * from the old Offsets and store into new offsets.
 * This application currently erases all the NVM values if the NVM version has
 * changed.
 */
#define APP_NVM_VERSION         (1)

/* Define this flag in the new image being created inorder to complete GAIA OTAu
 * procedure successfully if App/Stack offsets have changed between the old image
 * and new image.
 * Note: GAIA service offsets should be same between the two releases.
 * All NVM data will be lost after this update.
 */
//#define RESET_NVM

#define IMAGE_TYPE              (8)

/* Enable application debug logging on UART */
/* #define DEBUG_ENABLE */

/* Enable Static Random Address for bridge connectable advertisements */
/* #define USE_STATIC_RANDOM_ADDRESS */

/* Device name and its length
 * Note: Do not increase device name length beyond (DEFAULT_ATT_MTU -3 = 20) 
 * octets as GAP service at the moment doesn't support handling of Prepare 
 * write and Execute write procedures.
 */

/* Macro for the device name */
#define DEVICE_NAME                                 "Bridge2.1"

#define APPEARANCE                                  UNKNOWN

#define DISABLE_BEARER_SETTINGS

#ifdef CSR101x_A05
/* This flag should be enabled for supporting OTA on CSR101x Devices */
/* #define OTAU_BOOTLOADER */
#endif

/* Maximum Length of Device Name
 * Note: Do not increase device name length beyond (DEFAULT_ATT_MTU -3 = 20)
 * octets as GAP service at the moment doesn't support handling of Prepare
 * write and Execute write procedures.
 */
#define DEVICE_NAME_LENGTH                          (20)

/* Bonding Requirement */
#define GAP_MODE_BOND                               gap_mode_bond_no

/* Security Level Requirements */
#define GAP_MODE_SECURITY                   gap_mode_security_unauthenticate

/* Maximum active connections */
#define MAX_CONNECTIONS                             (1)

/* Maximum paired devices */
#define MAX_PAIRED_DEVICES                          (1)

/* Maximum number of server services supported */
#if defined(CSR101x_A05) && !defined(OTAU_BOOTLOADER)
#define MAX_SERVER_SERVICES                         (3)
#else
#define MAX_SERVER_SERVICES                         (4)
#endif

/* Initial diversifier */
#define DIVERSIFIER                                 (0)

/* Sourse and Sequence cache slot size */
#define SRC_SEQ_CACHE_SLOT_SIZE                     (0)

#endif /* __USER_CONFIG_H__ */

