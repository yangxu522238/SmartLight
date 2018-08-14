/******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      app_mesh_beacon_handler.c
 *
 *  DESCRIPTION
 *      This file implements the eddystone beacon advertisements
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <main.h>
#ifdef CSR101x_A05
#include <ls_app_if.h>
#include <config_store.h>
#include <gap_app_if.h>
#else
#include <gap_api.h>
#include <ls_api.h>
#endif
#include <timer.h>
#include <mem.h>
#include <random.h>
#include <gatt_uuid.h>
#include <debug.h>

/*============================================================================*
 *  Local Header File
 *============================================================================*/
#include "user_config.h"
#include "app_gatt.h"
#include "main_app.h"
#include "csr_mesh_model_common.h"
#include "app_mesh_handler.h"
#include "app_mesh_beacon_handler.h"
#include "advertisement_handler.h"
#include "app_debug.h"
/*============================================================================*
 *  Global Data
 *============================================================================*/
/*============================================================================*
 *  Local Defines
 *============================================================================*/
#define BEACON_INTERVAL_UNIT                    (10 * MILLISECOND)
/*============================================================================*
 *  Private Data
 *============================================================================*/
/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/
static void beaconTimerHandler(timer_id tid);
/*============================================================================*
 *  Private Function Definitions
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      generateRandomAddress
 *
 *  DESCRIPTION
 *      The function generates a non-resolvable private address.
 *
 *  RETURNS
 *      Nothing.
 *
 *----------------------------------------------------------------------------*/
static void generateRandomAddress(BD_ADDR_T *addr)
{
    /* "completely" random MAC addresses by default: */
    for(;;)
    {
        CsrUint32 now = TimeGet32();
        /* Random32() is just two of them, no use */
        CsrUint32 rnd = Random16();
        addr->uap = 0xff & (rnd ^ now);
        /* No sub-part may be zero or all-1s */
        if ( 0 == addr->uap || 0xff == addr->uap ) continue;
        addr->lap = 0xffffff & ((now >> 8) ^ (73 * rnd));
        if ( 0 == addr->lap || 0xffffff == addr->lap ) continue;
        addr->nap = 0x3fff & rnd;
        if ( 0 == addr->nap || 0x3fff == addr->nap ) continue;
        break;
    }
    /* Set it to actually be an acceptable random address */
    addr->nap &= ~BD_ADDR_NAP_RANDOM_TYPE_MASK;
    addr->nap |=  BD_ADDR_NAP_RANDOM_TYPE_NONRESOLV;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      setupBeaconPayload
 *
 *  DESCRIPTION
 *      Setup the advert data based on the various beacon specifcations
 *
 *  RETURNS
 *      Returns the length of the advertisement data.
 *---------------------------------------------------------------------------*/
static uint8 setupBeaconPayload(uint8 adv_data[], uint8 type, uint8 idx)
{
    uint16 offset = 0;
    int8 tx_power_level = 0xff; /* Signed value */

    /* MemSet the Advertisement data array */
    MemSet(adv_data, 0, MAX_USER_ADV_DATA_LEN);

    switch(type)
    {
        case csr_mesh_beacon_type_csr:
        {
            /* Length of the advertisement data type */
            adv_data[offset++] = 0x03;

            /* manufacturer-specific data */
            adv_data[offset++] = AD_TYPE_SERVICE_UUID_16BIT_LIST;

            /* EddyStone company code, must be 0x1903 little endian */
            adv_data[offset++] = CSR_RETAIL_UUID_LSB;
            adv_data[offset++] = CSR_RETAIL_UUID_MSB;

            /* Length of the advertisement data type */
            adv_data[offset++] = 0x02;

            /* manufacturer-specific data */
            adv_data[offset++] = AD_TYPE_TX_POWER;

            /* Read tx power of the chip */
            LsReadTransmitPowerLevel(&tx_power_level);

            /* Add the Tx Power Value */
            adv_data[offset++] = tx_power_level;

            /* Length of the advertisement data */
            adv_data[offset++] = 0x03 + 
                            CSR_RETAIL_BEACON_PAYLOAD_SIZE;

            /* manufacturer-specific data */
            adv_data[offset++] = AD_TYPE_MANUF;

            /* CSR company code */
            adv_data[offset++] = CSR_COMPANY_CODE_1;
            adv_data[offset++] = CSR_COMPANY_CODE_2;

            MemCopy(&adv_data[offset],
                    &g_beacon_handler_data.beacons[idx].payload[0],
                    CSR_RETAIL_BEACON_PAYLOAD_SIZE);

            offset += CSR_RETAIL_BEACON_PAYLOAD_SIZE;

            /* Length of the name */
            adv_data[offset++] = 0x05;

            /* Name */
            adv_data[offset++] = AD_TYPE_LOCAL_NAME_SHORT;

            /* EddyStone company code, must be 0x1903 little endian */
            adv_data[offset++] = 'R';
            adv_data[offset++] = 'e';
            adv_data[offset++] = 't';
            adv_data[offset++] = '\0';
        }
        break;

        case csr_mesh_beacon_type_ibeacon:
        {
            /* Length of the beacon data */
            adv_data[offset++] = 0x1A;

            /* manufacturer-specific data */
            adv_data[offset++] = AD_TYPE_MANUF;

            /* iBeacon company code, must be 0x004C little endian */
            adv_data[offset++] = IBEACON_COMPANY_CODE_1;
            adv_data[offset++] = IBEACON_COMPANY_CODE_2;

            /* Beacon Type = 0x02, Length = 0x15  */
            adv_data[offset++] = IBEACON_TYPE_1;
            adv_data[offset++] = IBEACON_TYPE_2;

            /* Ibeacon payload length is always set to 21, hence copy the 
             * complete 21 octets of data.
             */
            MemCopy(&adv_data[offset],
                    &g_beacon_handler_data.beacons[idx].payload[0],
                    IBEACON_PAYLOAD_SIZE);

            offset += IBEACON_PAYLOAD_SIZE;
        }
        break;

        case csr_mesh_beacon_type_eddystone_url:
        case csr_mesh_beacon_type_eddystone_uid:
        {
            eddystone_frame_type_t eddy_type;

            /* Length of the advertisement data type */
            adv_data[offset++] = 0x03;

            /* Service UUID List Type data */
            adv_data[offset++] = AD_TYPE_SERVICE_UUID_16BIT_LIST;

            /* EddyStone company code, must be 0xFEAA little endian */
            adv_data[offset++] = EDDYSTONE_UUID_LSB;
            adv_data[offset++] = EDDYSTONE_UUID_MSB;

            /* Length of the rest of advertisement data data */
            adv_data[offset++] = 0x04 + 
                        g_beacon_handler_data.beacons[idx].payloadlength;

            /* Service data */
            adv_data[offset++] = AD_TYPE_SERVICE_DATA_UUID_16BIT;

            /* EddyStone company code, must be 0xFEAA little endian */
            adv_data[offset++] = EDDYSTONE_UUID_LSB;
            adv_data[offset++] = EDDYSTONE_UUID_MSB;

            /* Add the relavant eddystone URL type */
            eddy_type = ((type == csr_mesh_beacon_type_eddystone_url)?
                           eddystone_frame_type_url : eddystone_frame_type_uid);
            adv_data[offset++] = eddy_type;

            MemCopy(&adv_data[offset],
                    &g_beacon_handler_data.beacons[idx].payload[0],
                    g_beacon_handler_data.beacons[idx].payloadlength);

            offset += g_beacon_handler_data.beacons[idx].payloadlength;
        }
        break;

        case csr_mesh_beacon_type_lte_direct:
        break;

        case csr_mesh_beacon_type_lumicast:
        break;

        default:
        break;
    }
    return offset;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      sendBeaconAdvert
 *
 *  DESCRIPTION
 *      The function adds a one shot beacon advertisement onto the CSRmesh 
 *      user advert to be scheduled and sent
 *
 *  RETURNS
 *      None
 *
 *---------------------------------------------------------------------------*/
static void sendBeaconAdvert(uint16 idx)
{
    CSR_SCHED_ADV_DATA_T gatt_adv_data;

    MemSet(&gatt_adv_data, 0, sizeof(CSR_SCHED_ADV_DATA_T));

    /* Restore the Random Address of the Bluetooth Device */
    MemCopy(&gatt_adv_data.adv_params.bd_addr.addr, 
            &g_beacon_handler_data.bd_addr, 
            sizeof(BD_ADDR_T));
    gatt_adv_data.adv_params.bd_addr.type = L2CA_RANDOM_ADDR_TYPE;

    /* Set GAP peripheral params */
    gatt_adv_data.adv_params.role = gap_role_broadcaster;
    gatt_adv_data.adv_params.bond = gap_mode_bond_no;
    gatt_adv_data.adv_params.connect_mode = gap_mode_connect_no;
    gatt_adv_data.adv_params.discover_mode = gap_mode_discover_no;
    gatt_adv_data.adv_params.security_mode = gap_mode_security_none;

    /* Setup the advertisement data and its length */
    gatt_adv_data.ad_data_length = 
            setupBeaconPayload(&gatt_adv_data.ad_data[0],
                                g_beacon_handler_data.beacons[idx].beacontype,
                                idx);

    /* Send the advert packet to mesh for scheduling */
    if(CSRSchedSendUserAdv(&gatt_adv_data, NULL) == CSR_SCHED_RESULT_SUCCESS)
    {
        DEBUG_STR("Beacon packet sent");
    }

    /* Start the beacon timer for sending periodic advertisements */
    TimerDelete(g_beacon_handler_data.beacons[idx].beacon_tid);

    g_beacon_handler_data.beacons[idx].beacon_tid = TimerCreate(
        g_beacon_handler_data.beacons[idx].beaconinterval * BEACON_INTERVAL_UNIT,
        TRUE, 
        beaconTimerHandler);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      beaconTimerHandler
 *
 *  DESCRIPTION
 *      This function handles the beacon timer expiry.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/
static void beaconTimerHandler(timer_id tid)
{
    uint8 index;
    for(index = 0; index < MAX_BEACONS_SUPPORTED; index++)
    {
        if (g_beacon_handler_data.beacons[index].beacon_tid == tid &&
            g_beacon_handler_data.beacons[index].beacontype 
                                                        != BEACON_TYPE_INVALID)
        {
            g_beacon_handler_data.beacons[index].beacon_tid = TIMER_INVALID;

            /* After the timer expiry send the periodic beacon */
            sendBeaconAdvert(index);
        }
    }
}

/*============================================================================*
 *  Public Function Definitions
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      StartStopBeaconing
 *
 *  DESCRIPTION
 *      The function is called to start/stop beaconing
 *
 *  RETURNS
 *      Nothing
 *---------------------------------------------------------------------------*/
extern void StartStopBeaconing(bool beacon_start)
{
    uint8 idx;

    /* configure the random bd address for the beacon adverrts */
    generateRandomAddress(&g_beacon_handler_data.bd_addr);

    if(beacon_start == TRUE)
    {
        for(idx = 0; idx < MAX_BEACONS_SUPPORTED; idx++)
        {
            if(g_beacon_handler_data.beacons[idx].beaconinterval > 0)
            {
                /* Start the beacon timers for sending periodic beacons */
                TimerDelete(g_beacon_handler_data.beacons[idx].beacon_tid);

                g_beacon_handler_data.beacons[idx].beacon_tid = TimerCreate(
                    g_beacon_handler_data.beacons[idx].beaconinterval *
                        BEACON_INTERVAL_UNIT,
                    TRUE, 
                    beaconTimerHandler);
            }
        }
        /* Stop the advertisements as we are entering the beacon mode */
        GattStopAdverts();
    }
    else
    {
        for(idx = 0; idx < MAX_BEACONS_SUPPORTED; idx++)
        {
            if(g_beacon_handler_data.beacons[idx].beacon_tid != TIMER_INVALID)
            {
                TimerDelete(g_beacon_handler_data.beacons[idx].beacon_tid);
                g_beacon_handler_data.beacons[idx].beacon_tid = TIMER_INVALID;
            }
        }
        /* Start the advertisements as we are leaving the beacon mode */
        GattTriggerConnectableAdverts(NULL);
    }
}
