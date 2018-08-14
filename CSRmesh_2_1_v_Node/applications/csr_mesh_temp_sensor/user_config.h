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

#define IMAGE_TYPE              (2)

#define CSR_MESH_SENSOR_PID     (0x1062)

/* Vendor ID for CSR */
#define APP_VENDOR_ID           (0x0A12)

/* Product ID. */
#define APP_PRODUCT_ID          (CSR_MESH_SENSOR_PID)

/* Number of model groups supported 
 * The switch application uses the Group ID assigned to the switch model as
 * the destination address for sending light control messages.
 * Since we only have one set of buttons(brightness control) and a switch(power 
 * control) on the IOT lighting board, to control destination devices, we
 * support only one group for all the supported models.
 */
#define MAX_MODEL_GROUPS        (4)

/* Version Number. */
#define APP_VERSION             (((uint32)(APP_MAJOR_VERSION & 0xFF) << 16) | \
                                 ((uint32)(APP_MINOR_VERSION & 0xFF)))

/* Default TTL value used in app */
#define DEFAULT_TTL_VALUE        (0x32)

/* Enable application debug logging on UART
#define DEBUG_ENABLE  */

/* Enable Static Random Address for bridge connectable advertisements */
/* #define USE_STATIC_RANDOM_ADDRESS */

/* Enable the this definition to use an authorisation code for association  */
#define USE_AUTHORISATION_CODE

/* Enable Device UUID Advertisements 
#define ENABLE_DEVICE_UUID_ADVERTS */

/* Enable Sensor model support */
#define ENABLE_SENSOR_MODEL

/* Enable Actuator model support */
#define ENABLE_ACTUATOR_MODEL

/* Enable Attention model support */
#define ENABLE_ATTENTION_MODEL

/* Enable Battery model support */
#define ENABLE_BATTERY_MODEL

/* Enable firmware model support */
#define ENABLE_FIRMWARE_MODEL

/* Temperature Sensor Parameters. */
/* STTS751 Temperature Sensor. */
#if defined(CSR101x_A05) && defined(NVM_TYPE_FLASH)
#else
#define TEMPERATURE_SENSOR_STTS751
#endif

/* Temperature Sensor Sampling Interval */
#define TEMPERATURE_SAMPLING_INTERVAL           (15 * SECOND)

/* Temperature Controller. */
#define ENABLE_TEMPERATURE_CONTROLLER

/* Duty Cycle support Parameters */
/* Enable duty cycle change support */
#define ENABLE_DUTY_CYCLE_CHANGE_SUPPORT

/* Default scan duty cycle in 0.1% with values ranging from 1-1000 */
#define DEFAULT_SCAN_DUTY_CYCLE                 (20)

/* scan duty cycle in 0.1% when device set to active scan mode. The device  
 * is present in this mode before grouping and on attention or data stream in 
 * progress.
 */
#define HIGH_SCAN_DUTY_CYCLE                    (1000)

/* Temperature Transmission Parameters */
/* Default repeat interval in seconds. This enables the sensor periodically
 * sending the temperature every repeat interval. Value range 0-255. The
 * interval within 1-30 seconds is considered to be min of 30 due to the 
 * retransmissions.
 */
#define DEFAULT_REPEAT_INTERVAL                 (0)

/* Number of msgs to be added in the transmit queue in one shot.
 * supported values 1-5
 */
#define TRANSMIT_MSG_DENSITY                    (2)

/* Number of msgs to be retransmitted per temp change */
#define NUM_OF_RETRANSMISSIONS                  (60)

/* Temperature change in 1/32 kelvin units. If temp changes more than this the
 * sensor would write the temp change onto the group.
 */
#define TEMPERATURE_CHANGE_TOLERANCE            (32)

/* Enable the Acknowledge mode */
/* #define ENABLE_ACK_MODE */

/* The below models are not enabled by default with constraint of space on the
 * CSR101x platform
 */
#ifndef CSR101x_A05
/* Battery threshold voltage */
#define BATTERY_THRESHOLD_VOLTAGE              (0x834)

/* Enable LOT model support */
#define ENABLE_LOT_MODEL 

/* Enable Ping model support */
#define ENABLE_PING_MODEL 

/* Enable Data model support */
#define ENABLE_DATA_MODEL 

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
#define MAX_ACTIONS_SUPPORTED                   (6)
#endif /* ENABLE_ACTION_MODEL */
#else
/* This flag should be enabled for supporting OTA on CSR101x Devices */
/* #define OTAU_BOOTLOADER */
#endif /* CSR101x_A05 */

/* Device name and its length
 * Note: Do not increase device name length beyond (DEFAULT_ATT_MTU -3 = 20) 
 * octets as GAP service at the moment doesn't support handling of Prepare 
 * write and Execute write procedures.
 */

/* Macro for the device name */
#define DEVICE_NAME                             "Sensor 2.1"

#define APPEARANCE                              UNKNOWN

/* Maximum Length of Device Name
 * Note: Do not increase device name length beyond (DEFAULT_ATT_MTU -3 = 20)
 * octets as GAP service at the moment doesn't support handling of Prepare
 * write and Execute write procedures.
 */
#define DEVICE_NAME_LENGTH                      (20)

/* Bonding Requirement */
#define GAP_MODE_BOND                           gap_mode_bond_no

/* Security Level Requirements */
#define GAP_MODE_SECURITY                       gap_mode_security_unauthenticate

/* Maximum active connections */
#define MAX_CONNECTIONS                         (1)

/* Maximum paired devices */
#define MAX_PAIRED_DEVICES                      (1)

/* Maximum number of server services supported */
#if defined(CSR101x_A05) && !defined(OTAU_BOOTLOADER)
#define MAX_SERVER_SERVICES                         (3)
#else
#define MAX_SERVER_SERVICES                         (4)
#endif

/* Initial diversifier */
#define DIVERSIFIER                             (0)

/* Sourse and Sequence cache slot size. If this value is changed ,
 * APP_NVM_VERSION needs to be changed keep the compatibility of
 * NVM contents with the application version
 */
#define SRC_SEQ_CACHE_SLOT_SIZE                 (0)

#ifdef ENABLE_LOT_MODEL
#define LOT_INTEREST_ADVERT_COUNT               (100)
#endif /* ENABLE_LOT_MODEL */

#endif /* __USER_CONFIG_H__ */

