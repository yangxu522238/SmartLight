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

#define IMAGE_TYPE              (0)

/* User Store Gaia OTAu ID */
#define USER_STORE_GAIA_OTAU_ID (3)

#define CSR_MESH_LIGHT_PID      (0x1060)

/* Vendor ID for CSR */
#define APP_VENDOR_ID           (0x0A12)

/* Product ID. */
#define APP_PRODUCT_ID          (CSR_MESH_LIGHT_PID)

/* Number of model groups supported */
#define MAX_MODEL_GROUPS        (4)

/* Version Number. */
#define APP_VERSION             (((uint32)(APP_MAJOR_VERSION & 0xFF) << 16) | \
                                ((uint32)(APP_MINOR_VERSION & 0xFF)))

/* Default TTL value used in app */
#define DEFAULT_TTL_VALUE        (0x32)

/* Enable application debug logging on UART */
//#define DEBUG_ENABLE 

#if !defined(DEBUG_ENABLE)
#define USE_ASSOCIATION_REMOVAL_KEY

/* Association Removal Button Press Duration */
#define LONG_KEYPRESS_TIME      (2 * SECOND)
#endif

/* Enable Static Random Address for bridge connectable advertisements 
#define USE_STATIC_RANDOM_ADDRESS */

/* Enable Light model support */
#define ENABLE_LIGHT_MODEL

/* Enable support for setting the color temperature */
#define COLOUR_TEMP_ENABLED 

/* Enable Power model support */
#define ENABLE_POWER_MODEL

/* Enable Attention model support */
#define ENABLE_ATTENTION_MODEL 

/* Enable Battery model support */
#define ENABLE_BATTERY_MODEL 

/* The below models are not enabled by default with constraint of space on the
 * CSR101x platform
 */
#ifndef CSR101x_A05
/* Battery threshold voltage */
#define BATTERY_THRESHOLD_VOLTAGE                 (0x834)

/* Enable Ping model support */
#define ENABLE_PING_MODEL 

/* Enable LOT model support */
#define ENABLE_LOT_MODEL 

/* Enable Data model support */
#define ENABLE_DATA_MODEL

/* Enable Tuning Model */
/* #define ENABLE_TUNING_MODEL */

/* Tuning Probe period */
#define DEFAULT_TUNING_PROBE_PERIOD               (10)

/* Tuning Report period */
#define DEFAULT_TUNING_REPORT_PERIOD              (60)

/* Enable Time model support */
#define ENABLE_TIME_MODEL

/* Enable Action model support */
#define ENABLE_ACTION_MODEL 

#if defined(ENABLE_ACTION_MODEL)
/* Enable Time model support */
#define ENABLE_TIME_MODEL 

/* Maximum Actions supported. Each action takes 24 bytes of data to be stored.
 * On increasing number of actions the NVM space for the same need to be 
 * increased accordingly.
 */
#define MAX_ACTIONS_SUPPORTED                     (6)
#endif /* ENABLE_ACTION_MODEL */

/* Enable Asset Model */
#define ENABLE_ASSET_MODEL

#if defined(ENABLE_ASSET_MODEL)
#define ASSET_SIDE_EFFECT_LIGHT_BMASK             (0x01)
#define ASSET_SIDE_EFFECT_AUDIO_BMASK             (0x02)
#define ASSET_SIDE_EFFECT_MOVEMENT_BMASK          (0x04)
#define ASSET_SIDE_EFFECT_STATIONARY_BMASK        (0x08)
#define ASSET_SIDE_EFFECT_HUMAN_BMASK             (0x10)

#define ASSET_SIDE_EFFECT_VALUE                   (ASSET_SIDE_EFFECT_LIGHT_BMASK)
#endif /* ENABLE_ASSET_MODEL */

/* Enable Tracker Model */
#define ENABLE_TRACKER_MODEL

#if defined(ENABLE_TRACKER_MODEL)
#define TRACKER_MAX_CACHED_ASSETS                 (10)
#define TRACKER_MAX_PENDING_ASSETS                (5)

/* Enabling this flag makes the tracker to store rssi based on rolling avg 
 * otherwise the most recently recived rssi is cached.
 */
#define TRACKER_CACHE_RSSI_ROLLING_AVG
#endif /* ENABLE_TRACKER_MODEL */

#else /* CSR101x_A05 */
/* Enable fast PWM using PIO controller instead of Hardware PWM */
#define ENABLE_FAST_PWM 

/* This flag should be enabled for supporting OTA on CSR101x Devices */
/* #define OTAU_BOOTLOADER */
#endif /* CSR101x_A05 */

/* Enable the this definition to use an authorisation code for association */
#define USE_AUTHORISATION_CODE 

/* Enable Device UUID Advertisements 
#define ENABLE_DEVICE_UUID_ADVERTS*/ 

/* Device name and its length
 * Note: Do not increase device name length beyond (DEFAULT_ATT_MTU -3 = 20) 
 * octets as GAP service at the moment doesn't support handling of Prepare 
 * write and Execute write procedures.
 */

/* Macro for the device name */
#define DEVICE_NAME                                 "Light2.1"

#define APPEARANCE                                  UNKNOWN

/* Maximum Length of Device Name
 * Note: Do not increase device name length beyond (DEFAULT_ATT_MTU -3 = 20)
 * octets as GAP service at the moment doesn't support handling of Prepare
 * write and Execute write procedures.
 */
#define DEVICE_NAME_LENGTH                          (20)

/* Bonding Requirement */
#define GAP_MODE_BOND                               gap_mode_bond_no

/* Security Level Requirements */
#define GAP_MODE_SECURITY                           gap_mode_security_unauthenticate

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

/* Sourse and Sequence cache slot size. If this value is changed ,
 * APP_NVM_VERSION needs to be changed keep the compatibility of
 * NVM contents with the application version
 */
#define SRC_SEQ_CACHE_SLOT_SIZE                     (0)

#ifdef ENABLE_LOT_MODEL
#define LOT_INTEREST_ADVERT_COUNT                   (100)
#endif /* ENABLE_LOT_MODEL */

#ifdef GAIA_OTAU_RELAY_SUPPORT
/* Connection event length minimum and maximum values
 * See Bluetooth core specification v4.1 for more information on connection
 * event lengths.
 */
#define CONN_EVENT_LENGTH_MIN                       (0)
#define CONN_EVENT_LENGTH_MAX                       (0)

/* Scan interval in number of slots */
#define SCAN_INTERVAL_SLOTS                         (600)

/* Scan window in number of slots. */
#define SCAN_WINDOW_SLOTS                           (400)

/* Choose appropriate values for scan interval and scan window. */
#define SCAN_INTERVAL                               (10 * MILLISECOND)
#define SCAN_WINDOW                                 (10 * MILLISECOND)

#define MAX_DISC_RETRY_COUNT                        (1)

#define DISCOVERY_TIMER                             (150 * MILLISECOND)

/* Maximum number of client services supported */
#define MAX_CLIENT_SERVICES                         (1)

#define ANNOUNCE_COUNT                              (8)

#define ANNOUNCE_COUNT_INTERVAL                     (5 * SECOND)

#define ANNOUNCE_INTERVAL                           (2 * MINUTE)

/* Max scan time */
#define SCAN_TIMEOUT                                (3 * MINUTE)

#define CONN_RETRY_COUNT                            (13)        

#define CONN_TIMEOUT                                (15 * SECOND)

#endif

#endif /* __USER_CONFIG_H__ */

