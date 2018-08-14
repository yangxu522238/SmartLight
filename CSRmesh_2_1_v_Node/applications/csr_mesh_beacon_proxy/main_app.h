/******************************************************************************
 *  Copyright 2015 - 2016 Qualcomm Technologies International, Ltd.
 *  Bluetooth Low Energy CSRmesh 2.1
 *  Application version 2.1.0
 *
 *  FILE
 *      main_app.h
 *
 *  DESCRIPTION
 *      Header definitions for CSR Mesh application file
 *
 ******************************************************************************/

#ifndef __MAIN_APP_H__
#define __MAIN_APP_H__

 /*============================================================================*
 *  SDK Header Files
 *============================================================================*/

/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "user_config.h"

/*============================================================================*
 *  CSR Mesh Header Files
 *============================================================================*/
#include "csr_mesh.h"
#include "csr_sched.h"
#include "cm_types.h"
#include "csr_mesh_nvm.h"
/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Configuration bits on the User Key */
#define CSKEY_RELAY_ENABLE_BIT         (1)
#define CSKEY_BRIDGE_ENABLE_BIT        (2)
#define CSKEY_RANDOM_UUID_ENABLE_BIT   (4)
#define UUID_LENGTH_WORDS              (8)
#define AUTH_CODE_LENGTH_WORDS         (4)

/* Maximum number of timers */
#define MAX_APP_TIMERS                 (20)

/* Magic value to check the sanity of NVM region used by the application */
#define NVM_SANITY_MAGIC               (0xAB96)

#define NVM_OFFSET_BEACON_MODEL_DATA   (CSR_MESH_NVM_SIZE)

#ifdef ENABLE_BEACON_MODEL
#define NVM_OFFSET_BEACON_MESH_TIME_IDX (NVM_OFFSET_BEACON_MODEL_DATA)

/* Three words for storing the beacon information for each supported beacon */
#define BEACON_SIZE                    (MAX_BEACON_PAYLOAD_SIZE/2 + 7)

/* Two words for storing beacon mesh information common across all beacons */
#define NVM_BEACON_SIZE                (MAX_BEACONS_SUPPORTED * BEACON_SIZE) + 1

/* Get NVM Offset of a specific action from its index. */
#define GET_BEACON_NVM_OFFSET(idx)     (NVM_OFFSET_BEACON_MODEL_DATA + 1 + \
                                       ((idx) * (BEACON_SIZE)))
#else
#define NVM_BEACON_SIZE                (0)
#endif

#define NVM_OFFSET_BEACON_PROXY_MODEL_DATA  (NVM_OFFSET_BEACON_MODEL_DATA + \
                                             NVM_BEACON_SIZE)

#ifdef ENABLE_BEACON_PROXY_MODEL
/* To Store Beacon Proxy devices and groups database */
#define NVM_BEACON_PROXY_SIZE          (MAX_MANAGED_BEACON_DEVS * 2) + \
                                        MAX_MANAGED_BEACON_GRPS

/* Get NVM Offset of the device id. */
#define GET_GRP_ID_NVM_OFFSET(idx)     (NVM_OFFSET_BEACON_PROXY_MODEL_DATA + \
                                       (idx))

#define GET_DEV_ID_NVM_OFFSET(idx)     (NVM_OFFSET_BEACON_PROXY_MODEL_DATA + \
                                       (MAX_MANAGED_BEACON_GRPS) + \
                                       (idx))

#define GET_DEV_GRP_BITMASK_NVM_OFFSET(idx) \
                                       (NVM_OFFSET_BEACON_PROXY_MODEL_DATA + \
                                       (MAX_MANAGED_BEACON_DEVS) + \
                                       (MAX_MANAGED_BEACON_GRPS) + \
                                       (idx))
#else
#define NVM_BEACON_PROXY_SIZE          (0)
#endif

#define NVM_OFFSET_TIME_INTERVAL       (NVM_OFFSET_BEACON_PROXY_MODEL_DATA + \
                                        NVM_BEACON_PROXY_SIZE)

#ifdef ENABLE_TIME_MODEL
#define NVM_TIME_INTERVAL_SIZE         (1)
#else
#define NVM_TIME_INTERVAL_SIZE         (0)
#endif

#define NVM_OFFSET_ACTION_MODEL_DATA   ((NVM_OFFSET_TIME_INTERVAL + \
                                         NVM_TIME_INTERVAL_SIZE))

#ifdef ENABLE_ACTION_MODEL
#define ACTION_SIZE                    (16)
#define NVM_ACTIONS_SIZE               (MAX_ACTIONS_SUPPORTED * ACTION_SIZE)

/* Get NVM Offset of a specific action from its index. */
#define GET_ACTION_NVM_OFFSET(idx)     (NVM_OFFSET_ACTION_MODEL_DATA + \
                                       ((idx) * (ACTION_SIZE)))
#else
#define NVM_ACTIONS_SIZE               (0)
#endif

#define NVM_OFFSET_ATT_MODEL_GROUPS    (NVM_OFFSET_ACTION_MODEL_DATA + \
                                        NVM_ACTIONS_SIZE)

#ifdef ENABLE_ATTENTION_MODEL
#define SIZEOF_ATT_MODEL_GROUPS       (sizeof(uint16)*MAX_MODEL_GROUPS)
#else
#define SIZEOF_ATT_MODEL_GROUPS         (0)
#endif /* ENABLE_ATTENTION_MODEL */

#define NVM_OFFSET_DATA_MODEL_GROUPS   (NVM_OFFSET_ATT_MODEL_GROUPS + \
                                        SIZEOF_ATT_MODEL_GROUPS)

#ifdef ENABLE_DATA_MODEL
#define SIZEOF_DATA_MODEL_GROUPS       (sizeof(uint16)*MAX_MODEL_GROUPS)
#else
#define SIZEOF_DATA_MODEL_GROUPS       (0)
#endif /* ENABLE_DATA_MODEL */

#define NVM_OFFSET_BEACON_MODEL_GROUPS   (NVM_OFFSET_DATA_MODEL_GROUPS + \
                                          SIZEOF_DATA_MODEL_GROUPS)

#ifdef ENABLE_BEACON_MODEL
#define SIZEOF_BEACON_MODEL_GROUPS       (sizeof(uint16)*MAX_MODEL_GROUPS)
#else
#define SIZEOF_BEACON_MODEL_GROUPS       (0)
#endif /* ENABLE_BEACON_MODEL */

#define NVM_OFFSET_LOT_MODEL_GROUPS    (NVM_OFFSET_BEACON_MODEL_GROUPS + \
                                        SIZEOF_BEACON_MODEL_GROUPS)

#ifdef ENABLE_LOT_MODEL
#define SIZEOF_LOT_MODEL_GROUPS        (sizeof(uint16)*MAX_MODEL_GROUPS)
#else
#define SIZEOF_LOT_MODEL_GROUPS        (0)
#endif /* ENABLE_LOT_MODEL */

/* NVM Offset for Application data */
#define NVM_MAX_APP_MEMORY_WORDS       (NVM_OFFSET_LOT_MODEL_GROUPS + \
                                        SIZEOF_LOT_MODEL_GROUPS)

uint16                                  g_app_nvm_offset;
bool                                    g_app_nvm_fresh;
uint16                                  g_cskey_flags;

#ifndef CSR101x
/* Cached Value of UUID. */
extern uint16 cached_uuid[UUID_LENGTH_WORDS];

/* Cached Value of Authorization Code. */
#ifdef USE_AUTHORISATION_CODE
extern uint16 cached_auth_code[AUTH_CODE_LENGTH_WORDS];
#endif /* USE_AUTHORISATION_CODE */

#ifdef RESET_NVM
bool                                    g_gaia_nvm_fresh;
#endif /* RESET_NVM */

#endif /* !CSR101x */

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/* Initialise application data */
extern void AppDataInit(void);

/* Initialise the Application supported services */
extern void InitAppSupportedServices(void);
#endif /* __MAIN_APP_H__ */

