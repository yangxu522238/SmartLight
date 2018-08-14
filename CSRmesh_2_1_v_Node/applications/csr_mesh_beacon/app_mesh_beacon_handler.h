/*******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      app_mesh_beacon_handler.h
 *
 *  DESCRIPTION
 *      This file defines the beacon related structures and definitions.
 *
 ******************************************************************************/
#ifndef __APP_MESH_BEACON_HANDLER_H__
#define __APP_MESH_BEACON_HANDLER_H__

/*=============================================================================*
 *  Application Header Files
 *============================================================================*/

#include "app_gatt.h"       /* Included panic codes */

/*=============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Eddystone beacon frame types */
typedef enum
{
    eddystone_frame_type_uid = 0x00,
    eddystone_frame_type_url = 0x10,
    eddystone_frame_type_tlm = 0x20
}eddystone_frame_type_t;

/* Eddystone service UUID */
#define EDDYSTONE_UUID                          (0xFEAA)
#define EDDYSTONE_UUID_LSB                      (0xAA)
#define EDDYSTONE_UUID_MSB                      (0xFE)

/* Csr Retail service UUID */
#define CSR_COMPANY_CODE_1                      (0x0A)
#define CSR_COMPANY_CODE_2                      (0x00)

#define CSR_RETAIL_UUID                         (0x1903)
#define CSR_RETAIL_UUID_LSB                     (0x03)
#define CSR_RETAIL_UUID_MSB                     (0x19)

/* Manufacturer ID of the beacon. This ID will be used for the AD_TYPE_MANUF to
 * insert the iBeacon Payload 
 */
#define IBEACON_COMPANY_CODE_1                  (0x4C)
#define IBEACON_COMPANY_CODE_2                  (0x00)
#define IBEACON_TYPE_1                          (0x02)
#define IBEACON_TYPE_2                          (0x15)

/*=============================================================================*
 *  Public Function Prototypes
 *============================================================================*/
/* This function should be used to start sending or stop sending the beacons */
extern void StartStopBeaconing(bool beacon_start);

#endif /* __APP_MESH_BEACON_HANDLER_H__ */

